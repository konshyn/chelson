#pragma once

namespace EventS
{
    class EventSubsystem
    {   
    public:
        EventSubsystem();
        ~EventSubsystem();
        bool Init();
        bool Finish();
    };
};