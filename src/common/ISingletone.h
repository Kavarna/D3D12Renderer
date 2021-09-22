#pragma once

#include <memory>
#include <mutex>
#include "CountParameters.h"
#include "TypeMatching.h"

template <typename type> class ISingletone {
protected:
    ISingletone() { };
    virtual ~ISingletone() {
        Reset();
    };

public:
    template <typename... Args> constexpr static type* Get(Args... args) {
        if constexpr (countParameters<Args...>::value == 0) {
            if constexpr (IsDefaultConstructible<type>::value) {
                std::call_once(m_singletoneFlags,
                               [&] { m_singletoneInstance = new type(args...); });
                return m_singletoneInstance;
            } else {
                return m_singletoneInstance;
            }
        } else {
            std::call_once(m_singletoneFlags,
                           [&] { m_singletoneInstance = new type(args...); });
            return m_singletoneInstance;
        }
    }

    static void Reset() {
        if (m_singletoneInstance) {
            auto ptr = m_singletoneInstance; // If we do it this way, we can safely
                                             // call reset() from destructor too
            m_singletoneInstance = nullptr;
            delete ptr;
        }
    };

private:
    static type* m_singletoneInstance;
    static std::once_flag m_singletoneFlags;
};

#define MAKE_SINGLETONE_CAPABLE(name) \
friend class ISingletone<name>;\
friend struct IsDefaultConstructible<name>;\

template <typename type>
type* ISingletone<type>::m_singletoneInstance = nullptr;
template <typename type>
std::once_flag ISingletone<type>::m_singletoneFlags;