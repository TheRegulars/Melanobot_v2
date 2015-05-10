/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LOCKING_REFERENCE_HPP
#define LOCKING_REFERENCE_HPP

#include "pointer_lock.hpp"

/**
 * \brief Base class for classes that provide a locking interface to another class
 */
template <class Lockable, class Referenced>
class LockingReferenceBase
{
public:
    LockingReferenceBase(Lockable* lock_target, Referenced* referenced)
        : lock_target(lock_target), referenced(referenced)
        {}

protected:
    auto lock() { return PointerLock<Lockable>(lock_target); }

    Lockable*  lock_target = nullptr;
    Referenced*referenced  = nullptr;
};

#endif // LOCKING_REFERENCE_HPP