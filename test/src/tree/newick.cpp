/**
 * @brief Testing Newick class.
 *
 * @file
 * @ingroup test
 */

#include "common.hpp"

#include <string>

#include "lib/tree/default/newick_processor.hpp"
#include "lib/tree/io/newick/processor.hpp"
#include "lib/tree/io/newick/color_mixin.hpp"
#include "lib/tree/tree.hpp"
#include "lib/utils/text/string.hpp"

using namespace genesis;

TEST(Newick, FromAndToString)
{
    std::string input = "((A,(B,C)D)E,((F,(G,H)I)J,K)L)R;";

    DefaultTree tree;
    EXPECT_TRUE(DefaultTreeNewickProcessor().from_string(input, tree));
    std::string output = DefaultTreeNewickProcessor().to_string(tree);

    EXPECT_EQ(input, output);
}

TEST(Newick, NewickVariants)
{
    DefaultTree tree;

    // No nodes are named.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(,,(,));",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // Leaf nodes are named.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(A,B,(C,D));",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // All nodes are named.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(A,B,(C,D)E)F;",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // All but root node have a distance to parent.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(:0.1,:0.2,(:0.3,:0.4):0.5);",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // All have a distance to parent.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(:0.1,:0.2,(:0.3,:0.4):0.5):0.0;",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // Distances and leaf names (popular).
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(A:0.1,B:0.2,(C:0.3,D:0.4):0.5);",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // Distances and all names.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "(A:0.1,B:0.2,(C:0.3,D:0.4)E:0.5)F;",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // A tree rooted on a leaf node (rare).
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "((B:0.2,(C:0.3,D:0.4)E:0.5)F:0.1)A;",
        tree
    ));
    EXPECT_TRUE(tree.validate());

    // All mixed, with comments and tags.
    EXPECT_TRUE( DefaultTreeNewickProcessor().from_string(
        "( ( Ant:0.2{0}, [a comment] 'Bee':0.09{1} )Inner:0.7{2}, Coyote:0.5{3} ){4};",
        tree
    ));
    EXPECT_TRUE(tree.validate());
}

TEST(Newick, ColorMixin)
{
    std::string input = "((A,(B,C)D)E,((F,(G,H)I)J,K)L)R;";

    DefaultTree tree;
    typedef DefaultTreeNewickMixin<NewickColorMixin<NewickProcessor<DefaultTree>>> ColorTreeNewickProcessor;

    // Make sure that the mixin does not interfere with other Newick functionality. If it does, the
    // following line would hopefully crash.
    EXPECT_TRUE( ColorTreeNewickProcessor().from_string(input, tree) );

    // Create a color vector for all edges that marks edges leading to a leaf node in red.
    auto color_vector = std::vector<Color>( tree.edge_count() );
    for( auto it = tree.begin_edges(); it != tree.end_edges(); ++it ) {
        if( (*it)->primary_node()->is_leaf() || (*it)->secondary_node()->is_leaf() ) {
            color_vector[(*it)->index()] = Color(255, 0, 0);
        }
    }

    // Use the color vector to produce a newick string with color tags.
    // We set ignored color to fuchsia ("magic pink") in order to also print out the black colored
    // inner edges.
    auto proc = ColorTreeNewickProcessor();
    proc.edge_colors(color_vector);
    proc.ignored_color(Color(255, 0, 255));
    std::string output = proc.to_string(tree);

    // Check if we actually got the right number of red color tag comments.
    auto count_red = text::count_substring_occurrences( output, "[&!color=#ff0000]" );
    EXPECT_EQ( tree.leaf_count(), count_red );

    // Check if we also got the right number of black color tag comments.
    // This is one fewer than the number of nodes, as no color tag is written for the root.
    auto count_black = text::count_substring_occurrences( output, "[&!color=#000000]" );
    EXPECT_EQ( tree.inner_count() - 1, count_black );
}
