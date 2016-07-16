// Copyright (c) 2015-2016 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#pragma once

#include <utility>
#include <ecst/config.hpp>
#include <ecst/aliases.hpp>

ECST_NAMESPACE
{
    namespace impl
    {
        using mutex_type = ecst::mutex;
        using cv_type = ecst::condition_variable;
        using counter_type = sz_t;
        using lock_guard_type = ecst::lock_guard<mutex_type>;
        using unique_lock_type = ecst::unique_lock<mutex_type>;

        /// @brief Accesses `cv` and `c` through a `lock_guard` on `mutex`, and
        /// calls `f(cv, c)`.
        template <typename TF>
        void access_cv_counter(
            mutex_type& mutex, cv_type& cv, counter_type& c, TF&& f) noexcept
        {
            lock_guard_type l(mutex);
            f(cv, c);

            // Prevent warnings.
            (void)l;
        }

        /// @brief Decrements `c` through `mutex`, and calls `f(cv)`.
        template <typename TF>
        void decrement_cv_counter_then(
            mutex_type& mutex, cv_type& cv, counter_type& c, TF&& f) noexcept
        {
            access_cv_counter(mutex, cv, c, [&f](auto& x_cv, auto& x_c)
                {
                    ECST_ASSERT_OP(x_c, >, 0);
                    --x_c;

                    f(x_cv);
                });
        }

        /// @brief Decrements `c` through `mutex`, and calls `cv.notify_one()`.
        void decrement_cv_counter_and_notify_one(
            mutex_type& mutex, cv_type& cv, counter_type& c) noexcept
        {
            decrement_cv_counter_then(mutex, cv, c, [](auto& x_cv)
                {
                    x_cv.notify_one();
                });
        }

        /// @brief Decrements `c` through `mutex`, and calls `cv.notify_all()`.
        void decrement_cv_counter_and_notify_all(
            mutex_type& mutex, cv_type& cv, counter_type& c) noexcept
        {
            decrement_cv_counter_then(mutex, cv, c, [](auto& x_cv)
                {
                    x_cv.notify_all();
                });
        }

        /// @brief Executes `f`, locks `mutex`, and waits until `predicate`
        /// is `true` through `cv`.
        template <typename TPredicate, typename TF>
        void execute_and_wait_until(mutex_type& mutex, cv_type& cv,
            TPredicate&& predicate, TF&& f) noexcept
        {
            f();

            unique_lock_type l(mutex);
            cv.wait(l, FWD(predicate));
        }

        /// @brief Locks `mutex`, executes `f` and waits until `c` is zero
        /// through `cv`.
        template <typename TF>
        void execute_and_wait_until_counter_zero(
            mutex_type& mutex, cv_type& cv, counter_type& c, TF&& f) noexcept
        {
            execute_and_wait_until(mutex, cv,
                [&c]
                {
                    return c == 0;
                },
                FWD(f));
        }
    }
}
ECST_NAMESPACE_END