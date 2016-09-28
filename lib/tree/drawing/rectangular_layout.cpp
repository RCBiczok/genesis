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

#include "tree/drawing/rectangular_layout.hpp"

#include "tree/default/distances.hpp"
#include "tree/default/tree.hpp"
#include "tree/function/distances.hpp"
#include "tree/function/functions.hpp"
#include "tree/function/operators.hpp"
#include "tree/iterator/eulertour.hpp"
#include "tree/iterator/preorder.hpp"
#include "tree/iterator/postorder.hpp"

#include "utils/core/logging.hpp"

#include <algorithm>
#include <assert.h>
#include <stdexcept>

namespace genesis {
namespace tree {

// =================================================================================================
//     Rectangular Layout
// =================================================================================================

RectangularLayout::RectangularLayout( Tree const& orig_tree )
{
    if( orig_tree.empty() ) {
        throw std::runtime_error( "Cannot draw an empty tree." );
    }

    // Get a copy of the tree topology.
    tree_ = orig_tree.clone_topology();

    // Init nodes.
    for( size_t i = 0; i < orig_tree.node_count(); ++i ) {
        // Safety: correct indices.
        assert( tree_.node_at(i).index() == i && orig_tree.node_at(i).index() == i );

        // Set the tree node data.
        tree_.node_at(i).reset_data( RectangularNodeData::create() );
        auto& node_data = tree_.node_at(i).data<RectangularNodeData>();

        // Set defaults needed to later distinguish whether those values have already been set.
        node_data.x = -1.0;
        node_data.y = -1.0;

        // If the original tree has node names, use them.
        auto orig_node_data_ptr = orig_tree.node_at(i).data_cast<DefaultNodeData>();
        if( orig_node_data_ptr ) {
            node_data.name = orig_node_data_ptr->name;
        }
    }

    // Init edges.
    for( size_t i = 0; i < orig_tree.edge_count(); ++i ) {
        // Safety: correct indices.
        assert( tree_.edge_at(i).index() == i && orig_tree.edge_at(i).index() == i );

        // Set the tree edge data.
        tree_.edge_at(i).reset_data( RectangularEdgeData::create() );
    }

    // Set node x-coords according to branch lengths (distance from root).
    // set_node_x_phylogram_();
    set_node_x_cladogram_();

    // Set node parents and y-coord of leaves.
    size_t leaf_count = 0;
    size_t parent = 0;
    for( auto it : eulertour( orig_tree )) {
        auto& node_data = tree_.node_at( it.node().index() ).data<RectangularNodeData>();

        if( node_data.parent_index == -1 ) {
            node_data.parent_index = parent;
        }
        if( it.node().is_leaf() ) {
            node_data.y = scaler_y_ * leaf_count++;
        }

        parent = it.node().index();
    }

    // Set remaining y-coords of inner nodes to mid-points of their children.
    for( auto it : postorder( orig_tree )) {
        auto& node_data   = tree_.node_at( it.node().index()      ).data<RectangularNodeData>();
        auto& parent_data = tree_.node_at( node_data.parent_index ).data<RectangularNodeData>();

        if( node_data.y < 0.0 ) {
            auto min_max_diff = node_data.children_max_y - node_data.children_min_y;
            node_data.y = node_data.children_min_y + min_max_diff / 2.0;
        }

        if( parent_data.children_min_y < 0.0 || parent_data.children_min_y > node_data.y ) {
            parent_data.children_min_y = node_data.y;
        }
        if( parent_data.children_max_y < 0.0 || parent_data.children_max_y < node_data.y ) {
            parent_data.children_max_y = node_data.y;
        }
    }
}

// -------------------------------------------------------------
//     Drawing
// -------------------------------------------------------------

void RectangularLayout::set_edge_strokes( std::vector< utils::SvgStroke > strokes )
{
    if( strokes.size() != tree_.edge_count() ) {
        throw std::runtime_error( "Edge stroke vector has wrong size." );
    }
    for( size_t i = 0; i < tree_.edge_count(); ++i ) {
        tree_.edge_at(i).data<RectangularEdgeData>().stroke = strokes[ i ];
    }
}

utils::SvgDocument RectangularLayout::to_svg_document() const
{
    using namespace utils;
    SvgDocument doc;

    for( auto const& node_it : tree_.nodes() ) {
        auto const& node = *node_it;

        auto const& node_data   = node.data<RectangularNodeData>();
        auto const& parent_data = tree_.node_at( node_data.parent_index ).data<RectangularNodeData>();

        // Get the edge between the node and its parent.
        auto edge_data_ptr = edge_between( node, tree_.node_at( node_data.parent_index ) );

        // If there is an edge (i.e., we are not at the root), draw lines between the nodes.
        if( edge_data_ptr ) {
            // Copy the stroke, because we are going to change it locally.
            auto stroke = edge_data_ptr->data<RectangularEdgeData>().stroke;
            stroke.line_cap = utils::SvgStroke::LineCap::kRound;

            doc << SvgLine(
                node_data.x, node_data.y,
                parent_data.x, node_data.y,
                stroke
            );
            doc << SvgLine(
                parent_data.x, node_data.y,
                parent_data.x, parent_data.y,
                stroke
            );
        } else {

            // If there is no edge, it must be the root.
            assert( node.is_root() );
        }

        auto label = SvgText( SvgPoint( node_data.x + 5, node_data.y ), node_data.name );
        label.dy = "0.4em";
        doc << label;
    }

    return doc;
}

void RectangularLayout::set_node_x_phylogram_()
{
    auto node_dists = node_branch_length_distance_vector( tree_ );

    for( size_t i = 0; i < node_dists.size(); ++i ) {
        tree_.node_at(i).data<RectangularNodeData>().x = node_dists[i] * scaler_x_;
    }
}

void RectangularLayout::set_node_x_cladogram_()
{
    // Set root x to 0.
    tree_.root_node().data<RectangularNodeData>().x = 0.0;

    // Get the height of the tree, i.e. longest path from root to any leaf.
    auto root_path_lengths = node_path_length_vector( tree_ );
    auto root_height       = *std::max_element( root_path_lengths.begin(), root_path_lengths.end() );

    for( auto it : preorder( tree_ )) {
        // The subtree height calculation does not work for the root, so skip it.
        // Also, we already set the leaf.
        if( it.is_first_iteration() ) {
            continue;
        }

        // Get the height of the subtree starting at the current node.
        auto height = subtree_max_path_height( tree_, it.link().outer() );
        assert( height <= root_height );

        // Set the x position.
        auto x = ( root_height - height ) * scaler_x_;
        tree_.node_at( it.node().index() ).data<RectangularNodeData>().x = x;
    }
}


} // namespace tree
} // namespace genesis
