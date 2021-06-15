#include "core.hpp"
#include "coordinator.hpp"
#include "parser.hpp"
#include "logger.hpp"
//运行2PC协议一致性地执行KV命令 
int Coordinator::Init(std::vector<NodeInfo> ps, NodeInfo c)
{
    int rc = KV_OK;

    // set the txid, conserve some for the recovery
    _TXID = TXID_START;

    // the init state is recovery
    _cstate = C_RECOVERY;

    // set the coordinator network for communication with clients
    _cnet = Network(c.port);

    // get p number
    _pnum = ps.size();

    _tmpNum = 0;

    for (p_id id = 1; id <= (p_id)_pnum; id++)
    {
        // init p's state is waiting
        _pstate[id] = P_WAITING;
        // set the p's network
        _pnet[id] = Network(ps[id - 1].add, ps[id - 1].port);
        // set the node info
        _pinfo[id] = ps[id - 1];

        _ptaskSem[id]   = cpLock(1);
        _pRetSem[id]    = cpLock(1);
        _pRtaskSem[id]  = cpLock(1);
        _pRRetSem[id]   = cpLock(1);
    }

    _recoveryTemp = cpLock(_tmpNum);

    return rc;
}

void Coordinator::pWorking(p_id id)
{
    assert(_pnet.count(id) == 1);

    while (1)
    {
        std::string rmsg = "";
        std::string task;
        int rc = KV_OK;

        _ptaskSem[id].cGet();
        _pRetSem[id].pGet();

        // the working
        if (_pstate[id] != P_WORKING)
        {
            _pworkingRet[id] = RET_SKIP;
            goto done;
        }

        task = _ptask[id];

        // send and recv from p
        rc = _pnet[id].sendAndRecv(task, rmsg);
        _pworkingRet[id] = rc;
        _pworkingDataRet[id] = rmsg;

        // any error occurs, close the socket and reconnet for recovering
        if (rc)
        {
            _pnet[id].close();
            _pstate[id] = P_WAITING;
        }

    done:
        _ptaskSem[id].cRelease();
        _pRetSem[id].pRelease();
    }
}

void Coordinator::keepAlive(p_id id)
{
    assert(_pnet.count(id) == 1);

    while (1)
    {

        if (_pstate[id] != P_WAITING)
        {
            continue;
        }

        std::cout << getTime() << _pinfo[id].add << ":" << _pinfo[id].port <<" CONNECTING..." << std::endl;
        // try to connect with the participant
        while (_pnet[id].reconnect())
        {
            // sleep(1);
        }

        std::cout << getTime() << _pinfo[id].add << ":" << _pinfo[id].port <<" CONNECTED" << std::endl;

        // change the state code
        _recoveryMutex.get();
        _pstate[id] = P_TEMP;
        _tmpNum++;
        _recoveryMutex.release();
    }
}

void Coordinator::pRecovery(p_id id)
{
    assert(_pnet.count(id) == 1);

    while (1)
    {
        std::string rmsg = "";
        std::string task;
        int rc = KV_OK;

        _pRtaskSem[id].cGet();
        _pRRetSem[id].pGet();
        // the working
        if (_pstate[id] != P_RECOVERY)
        {
            _precoveryRet[id] = RET_SKIP;
            goto done;
        }

        std::cout << getTime() << _pinfo[id].add << ":" << _pinfo[id].port <<" RECOVERY..." << std::endl;

        std::cout << getTime() << "TXID -> " << Log(_pRtask[id]).ID << std::endl;
        
        if (Log(_pRtask[id]).ID >= TXID_START)
        {
            if (Log(_pRtask[id]).ID != maxTxidTB[id] + 1)
            {
                _precoveryRet[id] = RET_SKIP;
                goto done;
            }
        }

        task = _pRtask[id];

        // send and recv from p
        rc = _pnet[id].sendAndRecv(task, rmsg);
        _precoveryRet[id] = rc;
        _preoveryDataRet[id] = rmsg;

        // any error occurs, close the socket and reconnet for recovering
        if (rc)
        {
            _pnet[id].close();
            _pstate[id] = P_WAITING;
            maxTxidTB.erase(id);
        }
        else
        {
            if (rmsg != _parser.getErrorMessage())
            {
                maxTxidTB[id] += 1;
            }
        }

    done:
        _pRRetSem[id].pRelease();
        _pRtaskSem[id].cRelease();
    }
}

/*
    The process is like:
        0. get the task from the client
        1. try to give the request to the all p and recv the response
        2. try to give the task to the all p and recv the
        3. then
            1. if the done get from any p, txid will increase by one
            2. or this resquest fails
        4. give back response
*/
int Coordinator::Working()
{
    std::cout << getTime() << "WORKING..." << std::endl;
    std::vector<std::string> task;
    while (_cnet.acceptWithoutCloseBind())
    {
        _cnet.initBind();
    }
    _cnet.recvCommand(task);

    std::string resp = _parser.getRESPArry(task);
    std::string rc = _parser.getErrorMessage();

    if (task.size() < 1)
    {
        rc = _parser.getErrorMessage();
        goto done;
    }

    while(_cstate != C_WORKING) {
        sleep(0.5); 
        goto done;
    }

    // lock
    _workingMutex.get();
    if (task[0] == "SET" || task[0] == "DEL")
    {
        rc = UpdateDB(resp);
    }
    else if (task[0] == "GET")
    {
        rc = Request(resp);
    }
    else
    {
        rc = _parser.getErrorMessage();
    }
    _workingMutex.release();

done:
    _cnet.sendResult(rc);
    sleep(0.1);
    _cnet.close();

    return KV_OK;
}

std::string Coordinator::UpdateDB(std::string resp)
{
    _lg.writeLog(_TXID, LOG_PRE, resp);

    // push the work into all the p
    std::string voteReq = Log(_TXID, LOG_PRE, "").logToStr();
    for (p_id id = 1; id <= _pnum; id++)
    {
        _ptaskSem[id].pGet();
        _ptask[id] = voteReq;
        _ptaskSem[id].pRelease();
    }

    bool vote = false;
    // get and check the result
    for (p_id id = 1; id <= _pnum; id++)
    {
        _pRetSem[id].cGet();
        // check the state
        if (_pworkingRet[id] == KV_OK &&
            _pworkingDataRet[id] == "PRE")
        {
            vote = true;
        }
        _pRetSem[id].cRelease();
    }

    // std::cout << "VOTE: " << vote << std::endl;

    // give the commit
    bool commit = false ;
    std::string rc = _parser.getErrorMessage();
    if (vote == true)
    {
        std::string commitReq = Log(_TXID, LOG_COMMIT, resp).logToStr();
        for (p_id id = 1; id <= _pnum; id++)
        {
            _ptaskSem[id].pGet();
            _ptask[id] = commitReq;
            _ptaskSem[id].pRelease();
        }

        std::string voteReq = Log(_TXID, LOG_PRE, "").logToStr();
        for (p_id id = 1; id <= _pnum; id++)
        {
            _pRetSem[id].cGet();
            // check the state
            if (_pworkingRet[id] == KV_OK &&
                _pworkingDataRet[id] != _parser.getErrorMessage())
            {
                commit = true;
                rc = _pworkingDataRet[id];
            }
            _pRetSem[id].cRelease();
        }
    }
    else
    {
        return _parser.getErrorMessage();
    }

    if (commit == true)
    {
        // push the log into the logger
        _lg.chageLogState(_TXID, LOG_COMMIT);
        _TXID++;
        return rc;
    }
    else
    {
        return _parser.getErrorMessage();
    }
}

std::string Coordinator::Request(std::string resp)
{
    std::string getReq = Log(SUPER_TXID, LOG_COMMIT, resp).logToStr();
    // push the work into all the p
    for (p_id id = 1; id <= _pnum; id++)
    {
        _ptaskSem[id].pGet();
        _ptask[id] = getReq;
        _ptaskSem[id].pRelease();

        _pRetSem[id].cGet();
        if (_pworkingRet[id] == KV_OK &&
            _pworkingDataRet[id] != "")
        {
            std::string rc = _pworkingDataRet[id];
            _pRetSem[id].cRelease();
            return rc;
        }
        _pRetSem[id].cRelease();
    }

    return _parser.getErrorMessage();
}

/*
    The process is like that:
        1. give a txid request to the all p to build a txid table
        2. cal the recovery task of each p
            -> 1. ask log 
            <- 2. get log
            -> 3. push log into the p
            <- 4. get done flag

    When recovery, I use a very simpley way:
        1. get the min TXID and max TXID
        2. ask for the old log from P which max-txid is max
        3. send the log to other p

*/
int Coordinator::Recovery()
{
    int rc = KV_OK;
    txid minID = _TXID - 1;
    maxTxidTB.clear();
    std::map<txid, p_id> task;
    Log txidReq = Log(RECOVERY_TXID, LOG_PRE, "");
    bool consistance = true;

    // waiting for more temp p
    while(_tmpNum == 0) {  sleep(0.1); }
    _cstate = C_RECOVERY;

    _recoveryMutex.get();
    _workingMutex.get();

    std::cout << "RECOVERY..." << std::endl;
    // put them into recovery mode
    for (auto &s : _pstate)
    {
        if (s.second == P_TEMP || s.second == P_WORKING)
        {
            s.second = P_RECOVERY;
        }
    }

    // ask for the max tixd of each p
    for (p_id id = 1; id <= _pnum; id++)
    {
        _pRtaskSem[id].pGet();
        _pRtask[id] = txidReq.logToStr();
        _pRtaskSem[id].pRelease();
    }

    // get max txid of each p
    for (p_id id = 1; id <= _pnum; id++)
    {
        _pRRetSem[id].cGet();
        // check the state
        if (_precoveryRet[id] == KV_OK &&
            _preoveryDataRet[id] != "")
        {
            
            maxTxidTB[id] = strtol(_preoveryDataRet[id].c_str(), nullptr, 10);
            if(maxTxidTB[id] != _TXID - 1) consistance = false;
        }
        _pRRetSem[id].cRelease();
    }
    
    if(!consistance) {
        for(auto entry : maxTxidTB) 
        {
            if(entry.second < minID) {
                minID = entry.second;
            }
        }

        // the next trasaction that I need to broadcast
        minID += 1;
        recoveryFromP(minID);
    }

    for (auto entry : maxTxidTB)
    {
        if(entry.second == _TXID - 1) {
            _pstate[entry.first] = P_WORKING;
        }
    }
    
    _tmpNum = 0;
    _workingMutex.release();
    _recoveryMutex.release();
    
     _cstate = C_WORKING;
    return rc;
}

void Coordinator::recoveryFromC(txid min)//从协调者恢复 
{
    for (txid tid = TXID_START; tid < _TXID; tid++)
    {
        // ask for the max tixd of each p
        for (p_id id = 1; id <= _pnum; id++)
        {
            _pRtaskSem[id].pGet();
            _pRtask[id] = _lg.getLogByTXID(tid).logToStr();
            _pRtaskSem[id].pRelease();

            _pRRetSem[id].cGet();
            _pRRetSem[id].cRelease();
        }
    }
}

void Coordinator::recoveryFromP(txid min)//从参与者恢复 
{
    txid minID = min;
    txid maxID = 0;
    p_id leader = getRecoveryLeader(maxID);

next:

    while (1)
    {
        leader = getRecoveryLeader(maxID);//不断请求最小事物ID+1，将其分发给对应节点，直到所有TXID都为max，结束，完成recovery 

        if (leader == 0)
        {
            recoveryFromC(minID);
            break;
        }

        // put leader into the working state
        _pstate[leader] = P_WORKING;
        maxTxidTB.erase(leader);

        // try to get data and send the data to other p
        for (; minID <= maxID; minID++)
        {
            // send the ask task
            std::string task = Log(ASK_DATA_TXID, LOG_COMMIT, std::to_string(minID)).logToStr();
            std::string rc = "";
            _ptaskSem[leader].pGet();
            _ptask[leader] = task;
            _ptaskSem[leader].pRelease();


            // get the log
            _pRetSem[leader].cGet();
            if (_pworkingRet[leader] == KV_OK &&
                _pworkingDataRet[leader] != "")
            {
                rc = _pworkingDataRet[leader];
            }
            else
            {
                _pRetSem[leader].cRelease();
                goto next;
            }
            _pRetSem[leader].cRelease();

            // for c's log
            Log tempLog;
            tempLog.strToLog(rc);
            if (tempLog.ID == _TXID)
            {
                _lg.writeLog(tempLog);
                _TXID++;
            }

            for (p_id id = 1; id <= _pnum; id++)
            {
                _pRtaskSem[id].pGet();
                _pRtask[id] = rc;
                _pRtaskSem[id].pRelease();

                _pRRetSem[id].cGet();
                _pRRetSem[id].cRelease();
            }
        }
    }
}

p_id Coordinator::getRecoveryLeader(txid & id)//找到最大事物ID，将其设为leader 
{
    txid maxID = _TXID - 1;
    p_id maxP = 0;
    for(auto entry : maxTxidTB) 
    {
        if(entry.second > maxID) {
            maxID = entry.second;
            maxP = entry.first;
        }
    }

    id = maxID;
    return maxP;
}

void Coordinator::work()
{
    while(1) 
    {
        Working();
    }
}

void Coordinator::recovery()
{
    while(1)
    {
        Recovery();
    }
}

int Coordinator::Launch()
{
    for(p_id id = 1; id <= _pnum; id++) {
        _talive[id] = std::thread(&Coordinator::keepAlive, this, id);
        _tworking[id] = std::thread(&Coordinator::pWorking, this, id);
        _trecovery[id] = std::thread(&Coordinator::pRecovery, this, id);
    }

     _workThread = std::thread(&Coordinator::work, this);

    // waiting all the p start for a while
    sleep(2);
    
    _recoveryThread = std::thread(&Coordinator::recovery, this);

    for(p_id id = 1; id <= _pnum; id++) {
        _talive[id].join();
        _tworking[id].join();
        _trecovery[id].join();
        _workThread.join();
        _recoveryThread.join();
    }
    return KV_OK;
}
