//
// Created by Monika on 28.01.2022.
//

#ifndef SR_ENGINE_SEMAPHORE_H
#define SR_ENGINE_SEMAPHORE_H

#include <Utils/Debug.h>

namespace SR_HTYPES_NS {
    class SR_DLL_EXPORT SignalSemaphore {
    public:
        SignalSemaphore()
            : m_lock(false)
        { }

        void Wait() {
            m_lock = true;
            while (m_lock);
        }

        void Signal() {
            m_lock = false;
        }

    private:
        std::atomic<bool> m_lock;

    };
}

#endif //SR_ENGINE_SEMAPHORE_H
