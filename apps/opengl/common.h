//!
//! This file contains headers of all external libraries
//!
#pragma once
#ifndef OSPRAY_COMMON_H
#define OSPRAY_COMMON_H
#define NOMINMAX
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
//
// include ospray
//
#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/utility/TransactionalValue.h"
using ospcommon::utility::TransactionalValue;

#endif //OSPRAY_COMMON_H
