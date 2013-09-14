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

//! Defines the default size of node indexing used in NeuroProof 
typedef unsigned int Index_t;

//! Defines location type used for 3 dimensional datasets
typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;

#endif
