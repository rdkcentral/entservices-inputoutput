/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
*
 * Copyright 2024 Synamedia Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
 */

#include "../Iarm.h"
#include "../thunder/Module.h"
#ifndef MODULE_NAME
#define MODULE_NAME RdkServicesL2Test
#endif
#include <core/core.h>
#include <interfaces/IIarm.h>
#include <cassert> 
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#define IARM_INSTANCE_INIT Instance().Initialize()

class IarmBusMockProxy {
private:
    class IarmNotification : public WPEFramework::Exchange::IIarm::INotification {
    private:
        IarmNotification(const IarmNotification&) = delete;
        IarmNotification& operator=(const IarmNotification&) = delete;

    public:
        explicit IarmNotification(IarmBusMockProxy& parent)
            : _parent(parent) {
        }
        ~IarmNotification() override = default;

    public:
        uint32_t IARM_EventHandler(const std::string& ownerName, int eventId, const std::string& data) override {
            _parent.IARM_EventHandler(ownerName, eventId, data);
            return 0;  
        }

        BEGIN_INTERFACE_MAP(IarmNotification)
        INTERFACE_ENTRY(WPEFramework::Exchange::IIarm::INotification)
        END_INTERFACE_MAP

    private:
        IarmBusMockProxy& _parent;
    };

public:
    class EXTERNAL Job : public WPEFramework::Core::IDispatch {
    protected:
        Job(IarmBusMockProxy *proxy, const std::string ownerName, int eventId, string &params)
            : _iarmBusMockProxy(proxy)
            ,_ownerName(ownerName)
            ,_eventId(eventId)
            , _params(params) {
        }

    public:
        Job() = delete;
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;
        ~Job() {
            _iarmBusMockProxy = nullptr;
        }

    public:
        static WPEFramework::Core::ProxyType< WPEFramework::Core::IDispatch> Create(IarmBusMockProxy *proxy, const std::string ownerName, int eventId,string params) 
        {
              assert(nullptr != proxy);
              return (WPEFramework::Core::ProxyType<WPEFramework::Core::IDispatch>(WPEFramework::Core::ProxyType<Job>::Create(proxy, ownerName ,eventId, params)));
        }

        virtual void Dispatch() override
        {
            try {
                assert(nullptr != _iarmBusMockProxy);
                _iarmBusMockProxy->Dispatch(_ownerName, _eventId, _params);
            } catch (const std::exception& e) {
                TEST_LOG("[Job::Dispatch] Exception caught during Dispatch: %s\n", e.what());
            }
        }

    private:
        IarmBusMockProxy *_iarmBusMockProxy;
        std::string _ownerName;
        int _eventId;
        const string _params;
    };
IarmBusMockProxy();  // Constructor
~IarmBusMockProxy();

static IarmBusMockProxy& Instance() {
    static IarmBusMockProxy instance;
    return instance;
}

public:
    // Interface methods
    static uint32_t IARM_EventHandler(const std::string& ownerName, int eventId, const std::string& data);
    static IARM_Result_t IarmBusInit(const char* name);
    static IARM_Result_t IarmBusConnect();
    static IARM_Result_t IarmBusIsConnected(const char* memberName, int* isRegistered);
    static IARM_Result_t IarmBusRegisterEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler);
    static IARM_Result_t IarmBusUnRegisterEventHandler(const char* ownerName, IARM_EventId_t eventId);
    static IARM_Result_t IarmBusRemoveEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler);
    static IARM_Result_t IarmBusCall(const char* ownerName, const char* methodName, void* arg, size_t argLen);
    static IARM_Result_t IarmBusBroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *arg, size_t argLen);
    static IARM_Result_t IarmBusRegisterCall(const char* methodName, IARM_BusCall_t handler);
    static IARM_Result_t IarmBusCallWithIPCTimeout(const char *ownerName, const char *methodName, void *arg, size_t argLen, int timeout);

    IARM_Result_t Initialize();
    // Static member to hold the implementation
    static WPEFramework::Exchange::IIarm* _mockPluginIarm;

private:
    WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>> _engine;
    WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient> _communicatorClient;

    WPEFramework::Core::Sink<IarmNotification> _notificationHandler;// TODO possible improvement: remove this variable, instead derive Proxy itself from INotification

    struct evt_handler_info
    {
        std::string owner;
        int event_id;
        IARM_EventHandler_t handler;
    };

    std::list<evt_handler_info> _eventHandlerInfos;

    void Dispatch(const std::string ownerName, int eventId, const string &data);
    void dispatchEvent(const std::string ownerName, int eventId,const string &params);
};


WPEFramework::Exchange::IIarm* IarmBusMockProxy::_mockPluginIarm = nullptr;

IARM_Result_t IarmBusMockProxy::Initialize() {
    TEST_LOG("Initializing COM RPC engine and communicator client");

    if (!_engine.IsValid()) {
        _engine = WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>>::Create();
        if (!_engine.IsValid()) {
            TEST_LOG("Failed to create _engine");
            return IARM_RESULT_INVALID_STATE;
        }
    }

    if (!_communicatorClient.IsValid()) {
        _communicatorClient = WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient>::Create(
            WPEFramework::Core::NodeId("/tmp/communicator"),
            WPEFramework::Core::ProxyType<WPEFramework::Core::IIPCServer>(_engine)
        );
        if (!_communicatorClient.IsValid()) {
            TEST_LOG("Failed to create _communicatorClient");
            return IARM_RESULT_INVALID_STATE;
        }
    }
        TEST_LOG("Connecting the COM-RPC socket");
        _mockPluginIarm = _communicatorClient->Open<WPEFramework::Exchange::IIarm>(_T("org.rdk.MockPlugin"), ~0, 3000);
        if (_mockPluginIarm) {
            TEST_LOG("Connected to MockPlugin successfully");
            _mockPluginIarm->Register(&(Instance()._notificationHandler));
        } else {
            TEST_LOG("Failed to create MockPlugin Controller");
        }

    return IARM_RESULT_SUCCESS;
}

IarmBusMockProxy::IarmBusMockProxy()
    : _notificationHandler(*this) {
    TEST_LOG("Inside IarmBusMockProxy constructor");
}

IarmBusMockProxy::~IarmBusMockProxy() {
    TEST_LOG("Inside IarmBusMockProxy destructor");
    if (_mockPluginIarm) {
        _mockPluginIarm->Unregister(&(Instance()._notificationHandler));
        _mockPluginIarm->Release();
        _mockPluginIarm = nullptr;
    }

    // Clear event handlers
    _eventHandlerInfos.clear();

    // Close the communicator client if it's valid
    if (_communicatorClient.IsValid()) {
        _communicatorClient->Close(WPEFramework::Core::infinite);
    }

    // Release the communicator client and engine
    _communicatorClient.Release();
    _engine.Release();
}

IARM_Result_t IarmBusMockProxy::IarmBusInit(const char* name)
{
    uint32_t status = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusInit");

    if (!_mockPluginIarm) {
        IARM_Result_t result = IARM_INSTANCE_INIT;
        if (result != IARM_RESULT_SUCCESS) {
            TEST_LOG("RPC initialize and connection failed");
            return result;
        }
    }

    if (_mockPluginIarm) {
        TEST_LOG("Calling IARM_Bus_Init on Mock Plugin");
        status =_mockPluginIarm->IARM_Bus_Init(name);

    } else {
        TEST_LOG("Mock Plugin is not initialized");
    }

    return (IARM_Result_t)status;
}

IARM_Result_t IarmBusMockProxy::IarmBusConnect()
{
    uint32_t status = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusConnect");

    if (!_mockPluginIarm) {
        IARM_Result_t result = IARM_INSTANCE_INIT;
        if (result != IARM_RESULT_SUCCESS) {
            TEST_LOG("RPC initialize and connection failed");
            return (IARM_Result_t)result;
        }
    }

    if (_mockPluginIarm) {
        TEST_LOG("Calling IARM_Bus_Connect on Mock Plugin");
        status =_mockPluginIarm->IARM_Bus_Connect();

    } else {
        TEST_LOG("Mock Plugin is not initialized");
    }

    return (IARM_Result_t)status;
}

IARM_Result_t IarmBusMockProxy::IarmBusIsConnected(const char* memberName, int* isRegistered)
{
    uint32_t status = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusIsConnected");
    if (!_mockPluginIarm) {
        IARM_Result_t result = IARM_INSTANCE_INIT;
        if (result != IARM_RESULT_SUCCESS) {
            TEST_LOG("RPC initialize and connection failed");
            return (IARM_Result_t)result;
        }
    }

    if (_mockPluginIarm) {
        std::string inputMessage = "Input message none"; 
        int outputResult = 0;
        TEST_LOG("Calling IARM_Bus_IsConnected on Mock Plugin");
        status =_mockPluginIarm->IARM_Bus_IsConnected(inputMessage, outputResult);
        TEST_LOG("IARM_Bus_IsConnected result: %u, output: %d", status, outputResult);
        *isRegistered = outputResult;
    } else {
        TEST_LOG("Mock Plugin is not initialized");
    }
    return (IARM_Result_t)status;
}

IARM_Result_t IarmBusMockProxy::IarmBusRegisterEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler)
{
    uint32_t registerResult = IARM_RESULT_INVALID_STATE;

    TEST_LOG("Calling _mockPluginIarm->Register on Mock Plugin");
    IarmBusMockProxy& instance = Instance();
    // Store the handler
    instance._eventHandlerInfos.push_back({ ownerName, eventId, handler });
    if (_mockPluginIarm) {
        registerResult = _mockPluginIarm->IARM_Bus_RegisterEventHandler(ownerName,eventId);
    } else {
        TEST_LOG("Dummy Plugin is not initialized");
    }

    TEST_LOG("Completed _mockPluginIarm->Register on Mock Plugin");
    return (IARM_Result_t)registerResult;
}

IARM_Result_t IarmBusMockProxy::IarmBusUnRegisterEventHandler(const char* ownerName, IARM_EventId_t eventId)
{
    uint32_t registerResult = IARM_RESULT_SUCCESS;
    TEST_LOG("Inside IarmBusUnRegisterEventHandler");

    return (IARM_Result_t)registerResult;
}

IARM_Result_t IarmBusMockProxy::IarmBusRemoveEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler)
{
    TEST_LOG("Inside IarmBusRemoveEventHandler");
    uint32_t result = _mockPluginIarm->IARM_Bus_RemoveEventHandler(ownerName, eventId);
    return (IARM_Result_t)result;
}

IARM_Result_t IarmBusMockProxy::IarmBusCall(const char* ownerName, const char* methodName, void* arg, size_t argLen)
{
    uint32_t result = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusCall");

    if (_mockPluginIarm) {
        result = _mockPluginIarm->IARM_Bus_Call(ownerName, methodName, reinterpret_cast<uint8_t*>(arg), argLen);
    } else {
        TEST_LOG("Dummy Plugin is not initialized");
    }

    return (IARM_Result_t)result;
}

IARM_Result_t IarmBusMockProxy::IarmBusBroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *arg, size_t argLen)
{
    uint32_t result = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusBroadcastEvent");

    if (_mockPluginIarm) {
        result = _mockPluginIarm->IARM_Bus_BroadcastEvent(ownerName, eventId, reinterpret_cast<uint8_t*>(arg), argLen);
    } else {
        TEST_LOG("Dummy Plugin is not initialized");
    }

    return (IARM_Result_t)result;
}

IARM_Result_t IarmBusMockProxy::IarmBusRegisterCall(const char* methodName, IARM_BusCall_t handler)
{
    uint32_t result = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusRegisterCall");

    if (_mockPluginIarm) {
        result = _mockPluginIarm->IARM_Bus_RegisterCall(methodName, reinterpret_cast<uint8_t*>(handler));
    } else {
        TEST_LOG("Dummy Plugin is not initialized");
    }

    return (IARM_Result_t)result;
}

IARM_Result_t IarmBusMockProxy::IarmBusCallWithIPCTimeout(const char *ownerName,  const char *methodName, void *arg, size_t argLen, int timeout)
{
    uint32_t result = IARM_RESULT_INVALID_STATE;
    TEST_LOG("Inside IarmBusCallWithIPCTimeout");

    if (_mockPluginIarm) {
        result = _mockPluginIarm->IARM_Bus_CallWithIPCTimeout(ownerName, methodName, reinterpret_cast<uint8_t*>(arg), argLen,timeout);
    } else {
        TEST_LOG("Dummy Plugin is not initialized");
    }

    return (IARM_Result_t)result;
}

void IarmBusMockProxy::dispatchEvent(const std::string ownerName, int eventId,  const string &params)
{
    WPEFramework::Core::IWorkerPool::Instance().Submit(Job::Create(this, ownerName,eventId, params));
}

void IarmBusMockProxy::Dispatch(const std::string ownerName, int eventId, const string &data)
{
    const void* dataPtr = static_cast<const void*>(data.data());
    size_t dataLength = data.size();
    bool handlerCalled = false;

    for (const auto& info : Instance()._eventHandlerInfos) 
    {
        if (info.owner == ownerName && info.event_id == eventId) 
        {
            TEST_LOG("Triggering event handler for owner: %s, eventId: %d, dataLength: %lu", ownerName.c_str(), eventId,dataLength);
            info.handler(ownerName.c_str(), eventId, const_cast<void*>(dataPtr), dataLength);
            handlerCalled = true; //continue with loop in case there are multiple handlers for this event
        }
    }

    if (!handlerCalled) 
    {
        TEST_LOG("No registered handler found for owner: %s, eventId: %d", ownerName.c_str(), eventId);
    }
}

uint32_t IarmBusMockProxy::IARM_EventHandler(const string& ownerName, int eventId, const string& data)
{
    bool handlerCalled = false;
    auto& instance = Instance();

    if (!instance._eventHandlerInfos.empty())
    {
        // Dispatch the event with the registered handler and data
        instance.dispatchEvent(ownerName,eventId, data);
    }
    return 0;
}

IARM_Result_t (*IARM_Bus_Init)(const char*) = &IarmBusMockProxy::IarmBusInit;
IARM_Result_t (*IARM_Bus_Connect)() = &IarmBusMockProxy::IarmBusConnect;
IARM_Result_t (*IARM_Bus_IsConnected)(const char*,int*) = &IarmBusMockProxy::IarmBusIsConnected;
IARM_Result_t (*IARM_Bus_RegisterEventHandler)(const char*,IARM_EventId_t,IARM_EventHandler_t) = &IarmBusMockProxy::IarmBusRegisterEventHandler;
IARM_Result_t (*IARM_Bus_UnRegisterEventHandler)(const char*,IARM_EventId_t) = &IarmBusMockProxy::IarmBusUnRegisterEventHandler;
IARM_Result_t (*IARM_Bus_RemoveEventHandler)(const char*,IARM_EventId_t,IARM_EventHandler_t) = &IarmBusMockProxy::IarmBusRemoveEventHandler;
IARM_Result_t (*IARM_Bus_Call)(const char*,const char*,void*,size_t) = &IarmBusMockProxy::IarmBusCall;
IARM_Result_t (*IARM_Bus_BroadcastEvent)(const char *ownerName, IARM_EventId_t eventId, void *arg, size_t argLen) = &IarmBusMockProxy::IarmBusBroadcastEvent;
IARM_Result_t (*IARM_Bus_RegisterCall)(const char*,IARM_BusCall_t) = &IarmBusMockProxy::IarmBusRegisterCall;
IARM_Result_t (*IARM_Bus_Call_with_IPCTimeout)(const char*,const char*,void*,size_t,int) = &IarmBusMockProxy::IarmBusCallWithIPCTimeout;


