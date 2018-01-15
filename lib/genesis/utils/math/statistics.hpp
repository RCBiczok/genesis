#ifndef GENESIS_UTILS_MATH_STATISTICS_H_
#define GENESIS_UTILS_MATH_STATISTICS_H_

/*
    Genesis - A toolkit for working with phylogenetic data.
    Copyright (C) 2014-2018 Lucas Czech and HITS gGmbH

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
 * @ingroup utils
 */

#include "genesis/utils/core/algorithm.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace genesis {
namespace utils {

// =================================================================================================
//     Forward Declarations
// =================================================================================================

// Needed for spearmans_rank_correlation_coefficient()
template <class RandomAccessIterator>
std::vector<double> ranking_fractional( RandomAccessIterator first, RandomAccessIterator last );

// =================================================================================================
//     Structures and Classes
// =================================================================================================

/**
 * @brief Store a pair of min and max values.
 *
 * This notation is simply more readable than the std default of using a `pair<T, T>`.
 */
template< typename T >
struct MinMaxPair
{
    T min;
    T max;
};

/**
 * @brief Store a mean and a standard deviation value.
 *
 * This notation is simply more readable than the std default of using a `pair<T, T>` for such
 * types.
 */
struct MeanStddevPair
{
    double mean;
    double stddev;
};

/**
 * @brief Store the values of quartiles: `q0 == min`, `q1 == 25%`, `q2 == 50%`, `q3 == 75%`,
 * `q4 == max`.
 */
struct Quartiles
{
    double q0 = 0.0;
    double q1 = 0.0;
    double q2 = 0.0;
    double q3 = 0.0;
    double q4 = 0.0;
};

// =================================================================================================
//     Mean Stddev
// =================================================================================================

/**
 * @brief Calculate the mean and standard deviation of a range of `double` elements.
 *
 * The iterators @p first and @p last need to point to a range of `double`. The function then
 * calculates the mean and standard deviation of all elements in the range that are finite.
 * If none are, or if the range is empty, both returned values are `0.0`.
 *
 * If the resulting standard deviation is below the given @p epsilon (e.g, `0.0000001`), it is
 * "corrected" to be `1.0` instead. This is an inelegant (but usual) way to handle near-zero values,
 * which for some use cases would cause problems like a division by zero later on.
 * By default, @p epsilon is `-1.0`, which deactivates this check - a standard deviation can never
 * be below `0.0`.
 */
template <class ForwardIterator>
MeanStddevPair mean_stddev( ForwardIterator first, ForwardIterator last, double epsilon = -1.0 )
{
    // Prepare result.
    MeanStddevPair result;
    result.mean   = 0.0;
    result.stddev = 0.0;
    size_t count = 0;

    // Sum up elements.
    auto it = first;
    while( it != last ) {
        if( std::isfinite( *it ) ) {
            result.mean += *it;
            ++count;
        }
        ++it;
    }

    // If there are no valid elements, return an all-zero result.
    if( count == 0 ) {
        return result;
    }

    //  Calculate mean.
    result.mean /= static_cast<double>( count );

    // Calculate column std dev.
    it = first;
    while( it != last ) {
        if( std::isfinite( *it ) ) {
            result.stddev += (( *it - result.mean ) * ( *it - result.mean ));
        }
        ++it;
    }
    assert( count > 0 );
    result.stddev /= static_cast<double>( count );
    result.stddev = std::sqrt( result.stddev );

    // The following in an inelegant (but usual) way to handle near-zero values,
    // which later would cause a division by zero.
    assert( result.stddev >= 0.0 );
    if( result.stddev <= epsilon ){
        result.stddev = 1.0;
    }

    return result;
}

/**
 * @brief Calculate the mean and standard deviation of a `vector` of `double` elements.
 *
 * See mean_stddev( ForwardIterator first, ForwardIterator last, double epsilon ) for details.
 */
inline MeanStddevPair mean_stddev( std::vector<double> const& vec, double epsilon = -1.0 )
{
    return mean_stddev( vec.begin(), vec.end(), epsilon );
}

// =================================================================================================
//     Median
// =================================================================================================

// TODO this weird range plus inidces implementation comes from before the whole functionw as a template. now, using just the range should be enough, so no need for the helper function!

/**
 * @brief Helper function to get the median in between a range. Both l and r are inclusive.
 */
template <class RandomAccessIterator>
double median( RandomAccessIterator first, RandomAccessIterator last, size_t l, size_t r )
{
    auto const size = static_cast<size_t>( std::distance( first, last ));
    assert( l < size && r < size && l <= r );

    // Size of the interval.
    size_t const sz = r - l + 1;

    // Even or odd size? Median is calculated differently.
    if( sz % 2 == 0 ) {

        // Get the two middle positions.
        size_t pl = l + sz / 2 - 1;
        size_t pu = l + sz / 2;
        assert( pl < size && pu < size );

        return ( *(first + pl) + *(first + pu) ) / 2.0;

    } else {

        // Int division, rounds down. This is what we want.
        size_t p = l + sz / 2;
        assert( p < size );

        return *(first + p);
    }
}

/**
 * @brief Calculate the median value of a range of `double`.
 *
 * The range has to be sorted, otherwise an exception is thrown.
 */
template <class RandomAccessIterator>
double median( RandomAccessIterator first, RandomAccessIterator last )
{
    // Checks.
    if( ! std::is_sorted( first, last )) {
        throw std::runtime_error( "Range has to be sorted for median calculation." );
    }
    auto const size = static_cast<size_t>( std::distance( first, last ));
    if( size == 0 ) {
        return 0.0;
    }

    // Use helper function, which takes the range inclusively.
    return median( first, last, 0, size - 1 );
}

/**
 * @brief Calculate the median value of a `vector` of `double`.
 *
 * The vector has to be sorted.
 */
inline double median( std::vector<double> const& vec )
{
    return median( vec.begin(), vec.end() );
}

// =================================================================================================
//     Quartiles
// =================================================================================================

template <class RandomAccessIterator>
Quartiles quartiles( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result.
    Quartiles result;

    // Checks.
    if( ! std::is_sorted( first, last )) {
        throw std::runtime_error( "Range has to be sorted for quartiles calculation." );
    }
    auto const size = static_cast<size_t>( std::distance( first, last ));
    if( size == 0 ) {
        return result;
    }

    // Set min, 50% and max.
    result.q0 = *first;
    result.q2 = median( first, last, 0, size - 1 );
    result.q4 = *(first + size - 1);

    // Even or odd size? Quartiles are calculated differently.
    // This could be done shorter, but this way feels more expressive.
    if( size % 2 == 0 ) {

        // Even: Split exaclty in halves.
        result.q1 = median( first, last, 0, size / 2 - 1 );
        result.q3 = median( first, last, size / 2, size - 1 );

    } else {

        // Odd: Do not include the median value itself.
        result.q1 = median( first, last, 0, size / 2 - 1 );
        result.q3 = median( first, last, size / 2 + 1, size - 1 );
    }

    return result;
}

/**
 * @brief Calculate the Quartiles of a `vector` of `double`.
 *
 * The vector has to be sorted.
 */
inline Quartiles quartiles( std::vector<double> const& vec )
{
    return quartiles( vec.begin(), vec.end() );
}

// =================================================================================================
//     Correlation Coefficients
// =================================================================================================

/**
 * @brief Calculate the Pearson Correlation Coefficient between two ranges of `double`.
 *
 * Both ranges need to have the same length. Then, the function calculates the PCC
 * between the pairs of entries of both ranges. It skipes entries where any of the two values
 * is not finite.
 *
 * If each pair of entries in the ranges contains at leat one non-finite value, that is, if there
 * are no pairs of finite values, a `quiet_NaN` is returned. Furtheremore, if one of the ranges
 * has a standard deviation of `0.0`, e.g., because all its entries are `0.0` themselves,
 * a division by 0 occurs, leading to a `NaN` as well.
 */
template <class ForwardIteratorA, class ForwardIteratorB>
double pearson_correlation_coefficient(
    ForwardIteratorA first_a, ForwardIteratorA last_a,
    ForwardIteratorB first_b, ForwardIteratorB last_b
) {
    // Calculate means.
    double mean_a = 0.0;
    double mean_b = 0.0;
    size_t count = 0;
    auto it_a = first_a;
    auto it_b = first_b;
    while( it_a != last_a && it_b != last_b ) {
        if( std::isfinite( *it_a ) && std::isfinite( *it_b ) ) {
            mean_a += *it_a;
            mean_b += *it_b;
            ++it_a;
            ++it_b;
            ++count;
        }
    }
    if( it_a != last_a || it_b != last_b ) {
        throw std::runtime_error( "Ranges need to have same length." );
    }
    if( count == 0 ) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    assert( count > 0 );
    mean_a /= static_cast<double>( count );
    mean_b /= static_cast<double>( count );

    // Calculate PCC parts.
    double numerator = 0.0;
    double stddev_a  = 0.0;
    double stddev_b  = 0.0;
    it_a = first_a;
    it_b = first_b;
    while( it_a != last_a && it_b != last_b ) {
        if( std::isfinite( *it_a ) && std::isfinite( *it_b ) ) {
            double const d1 = *it_a - mean_a;
            double const d2 = *it_b - mean_b;
            numerator += d1 * d2;
            stddev_a  += d1 * d1;
            stddev_b  += d2 * d2;
            ++it_a;
            ++it_b;
        }
    }
    assert( it_a == last_a && it_b == last_b );

    // Calcualte PCC, and assert that it is in the correct range
    // (or not a number, which can happen if the std dev is 0.0, e.g. in all-zero vectors).
    auto const pcc = numerator / ( std::sqrt( stddev_a ) * std::sqrt( stddev_b ) );
    assert(( -1.0 <= pcc && pcc <= 1.0 ) || ( ! std::isfinite( pcc ) ));
    return pcc;
}

/**
 * @brief Calculate the Pearson Correlation Coefficient between the entries of two vectors.
 *
 * See pearson_correlation_coefficient( ForwardIteratorA first_a, ForwardIteratorA last_a, ForwardIteratorB first_b, ForwardIteratorB last_b ) for details.
 */
inline double pearson_correlation_coefficient(
    std::vector<double> const& vec_a,
    std::vector<double> const& vec_b
) {
    return pearson_correlation_coefficient(
        vec_a.begin(), vec_a.end(), vec_b.begin(), vec_b.end()
    );
}

/**
 * @brief Calculate Spearman's Rank Correlation Coefficient between two ranges of `double`.
 *
 * Both ranges need to have the same length. Then, the function calculates Spearmans's Rho
 * between the pairs of entries of both vectors.
 *
 * Pairs of entries with contain non-finite values are skipped, see
 * pearson_correlation_coefficient( ForwardIteratorA first_a, ForwardIteratorA last_a, ForwardIteratorB first_b, ForwardIteratorB last_b ) for details.
 */
template <class RandomAccessIteratorA, class RandomAccessIteratorB>
double spearmans_rank_correlation_coefficient(
    RandomAccessIteratorA first_a, RandomAccessIteratorA last_a,
    RandomAccessIteratorB first_b, RandomAccessIteratorB last_b
) {
    // Get the ranking of both vectors.
    auto const ranks_a = ranking_fractional( first_a, last_a );
    auto const ranks_b = ranking_fractional( first_b, last_b );

    return pearson_correlation_coefficient( ranks_a, ranks_b );
}

/**
 * @brief Calculate Spearman's Rank Correlation Coefficient between the entries of two vectors.
 *
 * Both vectors need to have the same size. Then, the function calculates Spearmans's Rho
 * between the pairs of entries of both vectors.
 */
inline double spearmans_rank_correlation_coefficient(
    std::vector<double> const& vec_a,
    std::vector<double> const& vec_b
) {
    return spearmans_rank_correlation_coefficient(
        vec_a.begin(), vec_a.end(), vec_b.begin(), vec_b.end()
    );
}

/**
 * @brief Apply Fisher z-transformation to a correlation coefficient.
 *
 * The coefficient can be calculated with pearson_correlation_coefficient() or
 * spearmans_rank_correlation_coefficient() and has to be in range `[ -1.0, 1.0 ]`.
 *
 * There is also a version of this function for a vector of coefficients.
 * See also matrix_col_pearson_correlation_coefficient(),
 * matrix_row_pearson_correlation_coefficient(), matrix_col_spearmans_rank_correlation_coefficient()
 * and matrix_row_spearmans_rank_correlation_coefficient() for matrix versions.
 */
inline double fisher_transformation( double correlation_coefficient )
{
    auto const r = correlation_coefficient;
    if( r < -1.0 || r > 1.0 ) {
        throw std::invalid_argument(
            "Cannot apply fisher transformation to value " + std::to_string( r ) +
            " outside of [ -1.0, 1.0 ]."
        );
    }

    // LOG_DBG << "formula " << 0.5 * log( ( 1.0 + r ) / ( 1.0 - r ) );
    // LOG_DBG << "simple  " << std::atanh( r );
    return std::atanh( r );
}

/**
 * @brief Apply Fisher z-transformation to a vector of correlation coefficients.
 *
 * See fisher_transformation( double ) for details.
 */
inline std::vector<double> fisher_transformation( std::vector<double> const& correlation_coefficients )
{
    auto res = correlation_coefficients;
    for( auto& elem : res ) {
        elem = fisher_transformation( elem );
    }
    return res;
}

// =================================================================================================
//     Ranking Standard
// =================================================================================================

/**
 * @brief Return the ranking of the values in the given range, using Standard competition ranking
 * ("1224" ranking).
 *
 * See https://en.wikipedia.org/wiki/Ranking for details.
 *
 * @see ranking_modified(), ranking_dense(), ranking_ordinal(), ranking_fractional() for other
 * ranking methods.
 */
template <class RandomAccessIterator>
std::vector<size_t> ranking_standard( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result, and get the sorting order of the vector.
    auto const size = static_cast<size_t>( std::distance( first, last ));
    auto result = std::vector<size_t>( size, 1 );
    auto const order = stable_sort_indices( first, last );

    // Shortcuts for better readability.
    auto ordered_value = [&]( size_t i ){
        return *( first + order[i] );
    };
    auto ordered_result = [&]( size_t i ) -> size_t& {
        return result[ order[i] ];
    };

    // Calculate ranks.
    for( size_t i = 1; i < size; ++i ) {

        // Same values get the same rank. The next bigger one continues at the current i.
        if( ordered_value( i ) == ordered_value( i - 1 ) ) {
            ordered_result( i ) = ordered_result( i - 1 );
        } else {
            ordered_result( i ) = i + 1;
        }
    }

    return result;
}

/**
 * @copydoc ranking_standard( RandomAccessIterator first, RandomAccessIterator last )
 */
inline std::vector<size_t> ranking_standard( std::vector<double> const& vec )
{
    return ranking_standard( vec.begin(), vec.end() );
}

// =================================================================================================
//     Ranking Modified
// =================================================================================================

/**
 * @brief Return the ranking of the values in the given range, using Modified competition ranking
 * ("1334" ranking).
 *
 * See https://en.wikipedia.org/wiki/Ranking for details.
 *
 * @see ranking_standard(), ranking_dense(), ranking_ordinal(), ranking_fractional() for other
 * ranking methods.
 */
template <class RandomAccessIterator>
std::vector<size_t> ranking_modified( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result, and get the sorting order of the vector.
    auto const size = static_cast<size_t>( std::distance( first, last ));
    auto result = std::vector<size_t>( size, 1 );
    auto const order = stable_sort_indices( first, last );

    // Shortcuts for better readability.
    auto ordered_value = [&]( size_t i ){
        return *( first + order[i] );
    };
    auto ordered_result = [&]( size_t i ) -> size_t& {
        return result[ order[i] ];
    };

    // Calculate ranks. The loop variable is incremented at the end.
    for( size_t i = 0; i < size; ) {

        // Look ahead: How often does the value occur?
        size_t j = 1;
        while( i+j < size && ordered_value(i+j) == ordered_value(i) ) {
            ++j;
        }

        // Set the j-next entries.
        for( size_t k = 0; k < j; ++k ) {
            ordered_result( i + k ) = i + j;
        }

        // We can skip the j-next loop iterations, as we just set their values
        i += j;
    }

    return result;
}

/**
 * @copydoc ranking_modified( RandomAccessIterator first, RandomAccessIterator last )
 */
inline std::vector<size_t> ranking_modified( std::vector<double> const& vec )
{
    return ranking_modified( vec.begin(), vec.end() );
}

// =================================================================================================
//     Ranking Dense
// =================================================================================================

/**
 * @brief Return the ranking of the values in the given range, using Dense ranking ("1223" ranking).
 *
 * See https://en.wikipedia.org/wiki/Ranking for details.
 *
 * @see ranking_standard(), ranking_modified(), ranking_ordinal(), ranking_fractional() for other
 * ranking methods.
 */
template <class RandomAccessIterator>
std::vector<size_t> ranking_dense( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result, and get the sorting order of the vector.
    auto const size = static_cast<size_t>( std::distance( first, last ));
    auto result = std::vector<size_t>( size, 1 );
    auto const order = stable_sort_indices( first, last );

    // Shortcuts for better readability.
    auto ordered_value = [&]( size_t i ){
        return *( first + order[i] );
    };
    auto ordered_result = [&]( size_t i ) -> size_t& {
        return result[ order[i] ];
    };

    // Calculate ranks.
    for( size_t i = 1; i < size; ++i ) {

        // Same values get the same rank. The next bigger one continues by incrementing.
        if( ordered_value( i ) == ordered_value( i - 1 ) ) {
            ordered_result( i ) = ordered_result( i - 1 );
        } else {
            ordered_result( i ) = ordered_result( i - 1 ) + 1;
        }
    }

    return result;
}

/**
 * @copydoc ranking_dense( RandomAccessIterator first, RandomAccessIterator last )
 */
inline std::vector<size_t> ranking_dense( std::vector<double> const& vec )
{
    return ranking_dense( vec.begin(), vec.end() );
}

// =================================================================================================
//     Ranking Ordinal
// =================================================================================================

/**
 * @brief Return the ranking of the values in the given range, using Ordinal ranking ("1234" ranking).
 *
 * See https://en.wikipedia.org/wiki/Ranking for details.
 *
 * @see ranking_standard(), ranking_modified(), ranking_dense(), ranking_fractional() for other
 * ranking methods.
 */
template <class RandomAccessIterator>
std::vector<size_t> ranking_ordinal( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result, and get the sorting order of the vector.
    auto const size = static_cast<size_t>( std::distance( first, last ));
    auto result = std::vector<size_t>( size, 1 );
    auto const order = stable_sort_indices( first, last );

    // Shortcuts for better readability.
    auto ordered_result = [&]( size_t i ) -> size_t& {
        return result[ order[i] ];
    };

    // Calculate ranks. This is simply the order plus 1 (as ranks are 1-based).
    for( size_t i = 0; i < size; ++i ) {
        ordered_result( i ) = i + 1;
    }

    return result;
}

/**
 * @copydoc ranking_ordinal( RandomAccessIterator first, RandomAccessIterator last )
 */
inline std::vector<size_t> ranking_ordinal( std::vector<double> const& vec )
{
    return ranking_ordinal( vec.begin(), vec.end() );
}

// =================================================================================================
//     Ranking Fractional
// =================================================================================================

/**
 * @brief Return the ranking of the values in the given range, using Fractional ranking
 * ("1 2.5 2.5 4" ranking).
 *
 * See https://en.wikipedia.org/wiki/Ranking for details. This is the only raking method that
 * returns float values instead of integer values.
 *
 * @see ranking_standard(), ranking_modified(), ranking_dense(), ranking_ordinal() for other
 * ranking methods.
 */
template <class RandomAccessIterator>
std::vector<double> ranking_fractional( RandomAccessIterator first, RandomAccessIterator last )
{
    // Prepare result, and get the sorting order of the vector.
    auto const size = static_cast<size_t>( std::distance( first, last ));
    auto result = std::vector<double>( size, 1 );
    auto const order = stable_sort_indices( first, last );

    // Shortcuts for better readability.
    auto ordered_value = [&]( size_t i ){
        return *( first + order[i] );
    };
    auto ordered_result = [&]( size_t i ) -> double& {
        return result[ order[i] ];
    };

    // Calculate the average of the sum of numbers in the given inclusive range.
    auto sum_avg = []( size_t l, size_t r )
    {
        assert( l <= r );

        // Example:  l == 7, r == 9
        // We want:  (7 + 8 + 9) / 3 = 8.0
        // Upper:    1+2+3+4+5+6+7+8+9 = 45
        // Lower:    1+2+3+4+5+6       = 21
        // Diff:     45 - 21 = 24
        // Count:    9 - 7 + 1 = 3
        // Result:   24 / 3 = 8
        auto const upper = r * ( r + 1 ) / 2;
        auto const lower = ( l - 1 ) * l / 2;
        return static_cast<double>( upper - lower ) / static_cast<double>( r - l + 1 );
    };

    // Calculate ranks. The loop variable is incremented at the end.
    for( size_t i = 0; i < size; ) {

        // Look ahead: How often does the value occur?
        size_t j = 1;
        while( i+j < size && ordered_value(i+j) == ordered_value(i) ) {
            ++j;
        }

        // Set the j-next entries.
        auto entry = sum_avg( i + 1, i + j );
        for( size_t k = 0; k < j; ++k ) {
            ordered_result( i + k ) = entry;
        }

        // We can skip the j-next loop iterations, as we just set their values
        i += j;
    }

    return result;
}

/**
 * @copydoc ranking_fractional( RandomAccessIterator first, RandomAccessIterator last )
 */
inline std::vector<double> ranking_fractional( std::vector<double> const& vec )
{
    return ranking_fractional( vec.begin(), vec.end() );
}

} // namespace utils
} // namespace genesis

#endif // include guard
