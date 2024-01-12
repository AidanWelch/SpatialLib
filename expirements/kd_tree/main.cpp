// These tests are very ugly and just meant to compare performance
#include "./kd_tree_layer_optimized.hpp"
#include "./kd_tree_recursive.hpp"
#include "./kd_tree_recursive_template.hpp"
#include "./kd_tree_recursive_virtual.hpp"
#include "./kd_tree_stack_optimized.hpp"
#include "./kd_tree_stack_template.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

// NOLINTBEGIN(cert-msc30-c,cert-msc50-cpp,concurrency-mt-unsafe)

struct ArrayFloatData {
	int number;
	float coordinates[4];
};

struct VectorFloatData {
	int number;
	std::vector<float> coordinates;
};

struct ArrayIntData {
	int number;
	std::int32_t coordinates[4];
};

struct VectorIntData {
	int number;
	std::vector<std::int32_t> coordinates;
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
int stack_optimized_check = 0;
int layer_optimized_check = 0;
int recursive_check = 0;
int recursive_virtual_check = 0;
int recursive_template_check = 0;
int stack_template_check = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

template <typename T> int random_traverse( T node, std::int64_t seed ) {
	srand( seed );
	int total = 0;
	while ( true ) {
		total += node->data->number;
		if ( rand() % 2 && node->left != nullptr ) {
			node = node->left;
		} else if ( node->right != nullptr ) {
			node = node->right;
		} else {
			break;
		}
	}
	return total;
}

template <typename T>
std::uint64_t time_stack_optimized( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_stack_optimized::KD_Tree<T> tree( my_vector );
	std::uint64_t end = __rdtsc();
	stack_optimized_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

template <typename T>
std::uint64_t time_layer_optimized( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_layer_optimized::KD_Tree<T> tree( my_vector );
	std::uint64_t end = __rdtsc();
	layer_optimized_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

template <typename T> std::uint64_t time_recursive( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_recursive::KD_Tree<T> tree( my_vector );
	std::uint64_t end = __rdtsc();
	recursive_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

template <typename T>
std::uint64_t time_recursive_virtual( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_recursive_virtual::KD_Tree<T> tree( my_vector );
	std::uint64_t end = __rdtsc();
	recursive_virtual_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

template <typename T>
std::uint64_t time_recursive_template( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_recursive_template::KD_Tree tree( my_vector );
	std::uint64_t end = __rdtsc();
	recursive_template_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

template <typename T>
std::uint64_t time_stack_template( std::vector<T>& my_vector, std::int64_t seed ) {
	srand( seed );
	my_vector[0].coordinates[0] *= rand();
	std::uint64_t start = __rdtsc();
	spatial_lib_stack_template::KD_Tree tree( my_vector );
	std::uint64_t end = __rdtsc();
	stack_template_check += random_traverse( tree.nearest_neighbor(), seed );
	return end - start;
}

struct TestResults { // I know a much easier implementation is with an array but I have sunk the cost now
	// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
	std::uint64_t recursive_time = 0;
	std::uint64_t stack_optimized_time = 0;
	std::uint64_t layer_optimized_time = 0;
	std::uint64_t recursive_virtual_time = 0;
	std::uint64_t recursive_template_time = 0;
	std::uint64_t stack_template_time = 0;
	// NOLINTEND(misc-non-private-member-variables-in-classes)

	TestResults operator+( const TestResults& other ) {
		TestResults res = *this;
		res += other;
		return res;
	}

	TestResults& operator+=( const TestResults& other ) {
		recursive_time += other.recursive_time;
		stack_optimized_time += other.stack_optimized_time;
		layer_optimized_time += other.layer_optimized_time;
		recursive_virtual_time += other.recursive_virtual_time;
		recursive_template_time += other.recursive_template_time;
		stack_template_time += other.stack_template_time;
		return *this;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults& operator-=( T num ) {
		recursive_time -= num;
		stack_optimized_time -= num;
		layer_optimized_time -= num;
		recursive_virtual_time -= num;
		recursive_template_time -= num;
		stack_template_time -= num;
		return *this;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults& operator*=( T num ) {
		recursive_time *= num;
		stack_optimized_time *= num;
		layer_optimized_time *= num;
		recursive_virtual_time *= num;
		recursive_template_time *= num;
		stack_template_time *= num;
		return *this;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults operator*( T num ) {
		TestResults res = *this;
		res *= num;
		return res;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults operator-( T num ) {
		TestResults res = *this;
		res -= num;
		return res;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults operator/( T divisor ) {
		TestResults res = *this;
		res /= divisor;
		return res;
	}

	template <typename T>
		requires std::is_integral_v<T>
	TestResults& operator/=( T divisor ) {
		std::size_t cast_divisor = static_cast<std::size_t>( divisor );
		if ( divisor == 0 ) {
			throw std::invalid_argument( "Trying to divide TestResults by 0" );
		}
		recursive_time = static_cast<std::size_t>( recursive_time / cast_divisor );
		stack_optimized_time = static_cast<std::size_t>( stack_optimized_time / cast_divisor );
		layer_optimized_time = static_cast<std::size_t>( layer_optimized_time / cast_divisor );
		recursive_virtual_time = static_cast<std::size_t>( recursive_virtual_time / cast_divisor );
		recursive_template_time =
			static_cast<std::size_t>( recursive_template_time / cast_divisor );
		stack_template_time = static_cast<std::size_t>(stack_template_time / cast_divisor);
		return *this;
	}

	std::uint64_t min() {
		return std::min(
			{ recursive_time,
			  stack_optimized_time,
			  layer_optimized_time,
			  recursive_virtual_time,
			  recursive_template_time,
			  stack_template_time }
		);
	}
};

std::ostream& operator<<( std::ostream& stream, const TestResults& t ) {
	return stream << "recursive time:\t" << '\t' << t.recursive_time << '\n'
				  << "stack optimized time:" << '\t' << t.stack_optimized_time << '\n'
				  << "layer optimized time:" << '\t' << t.layer_optimized_time << '\n'
				  << "recursive virtual time:" << '\t' << t.recursive_virtual_time << '\n'
				  << "recursive template time:" << '\t' << t.recursive_template_time << '\n'
				  << "stack template time:" << '\t' << t.stack_template_time << '\n';
}

template <std::size_t S> struct ResultTable {
	constexpr static std::size_t columns = S;
	std::array<std::string, S> headers;
	std::array<TestResults, S> results;

	ResultTable<S>( std::array<std::string, S> headers, std::array<TestResults, S> results )
		: headers( headers ), results( results ){};
};

template <std::size_t S> std::ostream& operator<<( std::ostream& os, const ResultTable<S>& t ) {
	std::array<std::stringstream, 7> rows;
	rows[0] << std::setw( 25 ) << ' ';
	rows[1] << std::setw( 25 ) << "recursive time:";
	rows[2] << std::setw( 25 ) << "stack optimized time:";
	rows[3] << std::setw( 25 ) << "layer optimized time:";
	rows[4] << std::setw( 25 ) << "recursive virtual time:";
	rows[5] << std::setw( 25 ) << "recursive template time:";
	rows[6] << std::setw( 25 ) << "stack template time:";
	for ( std::size_t i = 0; i < t.columns; i++ ) {
		rows[0] << std::setw( 15 ) << t.headers[i] << std::setw( 1 ) << '|';
		rows[1] << std::setw( 15 ) << t.results[i].recursive_time << std::setw( 1 ) << '|';
		rows[2] << std::setw( 15 ) << t.results[i].stack_optimized_time << std::setw( 1 ) << '|';
		rows[3] << std::setw( 15 ) << t.results[i].layer_optimized_time << std::setw( 1 ) << '|';
		rows[4] << std::setw( 15 ) << t.results[i].recursive_virtual_time << std::setw( 1 ) << '|';
		rows[5] << std::setw( 15 ) << t.results[i].recursive_template_time << std::setw( 1 ) << '|';
		rows[6] << std::setw( 15 ) << t.results[i].stack_template_time << std::setw( 1 ) << '|';
	}
	std::stringstream result;
	for ( const std::stringstream& row : rows ) {
		result << row.str() << '\n';
	}
	return os << result.str();
}

std::uint64_t distance_from_median( std::vector<std::uint64_t> vec, std::size_t i ) {
	const std::size_t midpoint = vec.size() / 2;
	if ( vec[midpoint] > vec[i] ) {
		return vec[midpoint] - vec[i];
	}
	return vec[i] - vec[midpoint];
}

template <typename T>
TestResults test_creation_for_all( const int iterations, std::size_t vector_size, T data_container ) {
	const std::size_t TIME_COUNT = 6;
	std::array<std::vector<std::uint64_t>, TIME_COUNT> all_times;
	std::cout << "KD Tree Creation" << '\n';
	std::cout << "Iterations: " << iterations;
	std::cout << " Data length: " << vector_size << '\n';
	std::cout << "[ " << std::flush;
	const int marker_count = 25;
	srand( time( nullptr ) );
	for ( int i = 0; i < iterations; i++ ) {
		int seed = rand();
		std::array<std::uint64_t, TIME_COUNT> times = {
			time_recursive( data_container, seed ),
			time_stack_optimized( data_container, seed ),
			time_layer_optimized( data_container, seed ),
			time_recursive_virtual( data_container, seed ),
			time_recursive_template( data_container, seed ),
			time_stack_template( data_container, seed )
		};
		for ( std::size_t j = 0; j < all_times.size(); j++ ) {
			all_times[j].emplace_back( times[j] );
		}

		if ( i % ( iterations / marker_count ) == 0 ) {
			std::cout << "=" << std::flush;
		}
	}
	std::cout << " ]\n";
	std::cout << "------------------------------------------------" << '\n'
			  << "Boundary times: " << '\n';
	for ( std::vector<std::uint64_t> t : all_times ) {
		std::sort( t.begin(), t.end() );
	}
	TestResults minimums = {
		all_times[0].back(),
		all_times[1].back(),
		all_times[2].back(),
		all_times[3].back(),
		all_times[4].back(),
		all_times[5].back()
	};
	TestResults maximums = {
		all_times[0].front(),
		all_times[1].front(),
		all_times[2].front(),
		all_times[3].front(),
		all_times[4].front(),
		all_times[5].front()
	};
	ResultTable maximum_table = {
		std::array<std::string, 3>( { "Minimum", "Max", "Max Difference" } ),
		std::array<TestResults, 3>( { minimums, maximums, maximums - maximums.min() } )
	};
	std::cout << maximum_table << std::flush;
	std::cout << "------------------------------------------------" << '\n'
			  << "Median times: " << '\n';
	const std::size_t midpoint = iterations / 2;
	TestResults results = {
		all_times[0][midpoint],
		all_times[1][midpoint],
		all_times[2][midpoint],
		all_times[3][midpoint],
		all_times[4][midpoint],
		all_times[5][midpoint]
	};

	TestResults median_absolute_deviation;
	// Can be disabled if its too slow
	for (std::size_t i = 0; i < iterations; i++ ) {
		median_absolute_deviation += {distance_from_median(all_times[0], i), distance_from_median(all_times[1], i), distance_from_median(all_times[2], i), distance_from_median(all_times[3], i), distance_from_median(all_times[4], i), distance_from_median(all_times[5], i)};
	}
	median_absolute_deviation /= iterations;
	// Just comment to here

	ResultTable results_table = {
		std::array<std::string, 3>( { "Median", "Difference", "Med.A.D." } ),
		std::array<TestResults, 3>( { results, results - results.min(), median_absolute_deviation }
		)
	};
	std::cout << results_table << std::flush;
	return results;
}

void compare_creations_for_all() {
	TestResults totals;
	TestResults totals_per_100;
	std::array<TestResults, 10> median_results;
	std::array<std::string, 10> vector_size_headers;
	for ( std::size_t i = 0; i < 10; i++ ) {
		std::cout << "################## ARRAY INT ################### " << i + 1 << "/10" << '\n'
				  << std::flush;
		std::size_t vector_size = 200000 - ( 20000 * i );

		std::vector<ArrayIntData> array_int_vector;
		std::vector<VectorIntData> dynamic_int_vector;
		for ( std::int32_t j = 0; j < static_cast<std::int32_t>( vector_size ); j++ ) {
			array_int_vector.push_back( { j, { j, ( 2 * j ), ( 3 * j ), ( 4 * j ) } } );
			dynamic_int_vector.emplace_back(
				i, std::initializer_list<std::int32_t>( { j, ( 2 * j ), ( 3 * j ), ( 4 * j ) } )
			);
		}

		vector_size_headers[i] = std::to_string( vector_size );
		TestResults results = test_creation_for_all( 25 * ( i + 1 ), vector_size, array_int_vector );
		median_results[i] = results;
		totals_per_100 += results * 100 / vector_size;
		totals += results;
		std::cout << "################################################" << '\n'
				  << "Rolling Average: " << '\n';
		TestResults temp = totals;
		TestResults temp_per_100 = totals_per_100;
		temp /= i + 1;
		temp -= temp.min();
		temp_per_100 /= i + 1;
		temp_per_100 -= temp_per_100.min();
		ResultTable rolling_table = {
			std::array<std::string, 2>( { "Median Diff", "Per 100 Nodes" } ), std::array<TestResults, 2>( { temp, temp_per_100 } )
		};
		std::cout << rolling_table << std::flush;
	}
	ResultTable median_results_table = { vector_size_headers, median_results };
	std::cout << "################################################" << '\n'
			  << "Median Totals for data size: " << '\n';
	std::cout << median_results_table << std::flush;
	if ( recursive_check != stack_optimized_check ||
		stack_optimized_check != layer_optimized_check ||
		layer_optimized_check != recursive_virtual_check ||
		recursive_virtual_check != recursive_template_check  || recursive_template_check != stack_template_check) {
		std::cout << "################################################"
				  << "CHECKS WRONG!!: " << '\n'
				  << "recursive:         " << recursive_check << '\n'
				  << "stack optimized:   " << stack_optimized_check << '\n'
				  << "layer optimized:   " << layer_optimized_check << '\n'
				  << "recursive virtual: " << recursive_virtual_check << '\n'
				  << "recursive template: " << recursive_template_check << '\n'
				  << "stack template: " << stack_template_check << '\n'
				  << std::flush;
	}
}

int main() {
	compare_creations_for_all();
	return 0;
}
// NOLINTEND(cert-msc30-c,cert-msc50-cpp,concurrency-mt-unsafe)