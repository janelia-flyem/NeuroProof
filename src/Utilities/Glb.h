/*!
 * \file
 *
 * Provide definitions for different global parameters.
 *
 * \author Stephen Plaza (plaza.stephen@gmail.com)
 *
*/

#ifndef GLB_H
#define GLB_H

#include <boost/tuple/tuple.hpp>
#include <boost/cstdint.hpp>

namespace NeuroProof {

typedef boost::uint8_t uint8;
typedef boost::uint32_t uint32;
typedef boost::int32_t int32;
typedef boost::uint64_t uint64;

//! Defines the default size of node indexing used in NeuroProof 
typedef uint32 Index_t;

//! Defines location type used for 3 dimensional datasets
typedef boost::tuple<uint32, uint32, uint32> Location;


} // namespace NeuroProof

#endif
