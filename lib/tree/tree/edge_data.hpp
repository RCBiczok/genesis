#ifndef GENESIS_TREE_TREE_EDGE_DATA_H_
#define GENESIS_TREE_TREE_EDGE_DATA_H_

/*
    Genesis - A toolkit for working with phylogenetic data.
    Copyright (C) 2014-2016 Lucas Czech

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact:
    Lucas Czech <lucas.czech@h-its.org>
    Exelixis Lab, Heidelberg Institute for Theoretical Studies
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

/**
 * @brief
 *
 * @file
 * @ingroup tree
 */

#include "utils/core/std.hpp"

#include <memory>

namespace genesis {
namespace tree {

// =================================================================================================
//     Forward declarations
// =================================================================================================

class Tree;
class TreeNode;
class TreeEdge;
class TreeLink;

// =================================================================================================
//     Tree Edge Data Base
// =================================================================================================

class BaseEdgeData
{
public:

    BaseEdgeData() = default;
    virtual ~BaseEdgeData()
    {}

    virtual std::unique_ptr< BaseEdgeData > clone() const
    {
        // return utils::make_unique< BaseEdgeData >( *this );
        return std::unique_ptr< BaseEdgeData >( new BaseEdgeData( *this ));
    };

    // Move ctor and assignment.
    BaseEdgeData( BaseEdgeData&& other )             = delete;
    BaseEdgeData& operator= ( BaseEdgeData&& other ) = delete;

protected:

    // Copy ctor and assignment.
    BaseEdgeData( BaseEdgeData const& other )             = default;
    BaseEdgeData& operator= ( BaseEdgeData const& other ) = default;

};

} // namespace tree
} // namespace genesis

#endif // include guard
