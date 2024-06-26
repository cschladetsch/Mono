// (C) 2009 Christian Schladetsch
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// documentation at https://svn.boost.org/svn/boost/sandbox/monotonic/libs/monotonic/doc/index.html
// sandbox at https://svn.boost.org/svn/boost/sandbox/monotonic/

#include <iostream> 
#include <iomanip> 
#include <numeric>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <bitset>
#include <string>
#include <math.h>

#include <monotonic/containers/string.hpp>
#include <iterator>
#include <boost/timer/timer.hpp>

#include "./AllocatorTypes.h"
#include "./PoolResult.h"
#include "./Tests.h"

#include <monotonic/storage.hpp>

#ifdef WIN32
    //warning C4996: 'std::fill_n': Function call with parameters that may be unsafe
    #pragma warning(disable:4996)
    //warning C4267: 'argument': conversion from 'size_t' to 'int', possible loss of data
    #pragma warning(disable:4267)
#endif

using namespace std;
using namespace boost;
using namespace boost::timer;

struct my_local {};

vector<PoolResult> cumulative;
PoolResult result_min, result_max;
vector<int> random_numbers;
vector<pair<int, int> > random_pairs;
bool first_result = true;

// ensure tests for different allocators get the same random number sequences
void SeedRand()
{
    srand(42);
}

template <class Fun>
PoolResult run_test(size_t count, size_t length, Fun fun, Type types)
{
    typedef typename Allocator<Type::Monotonic, int> mono_alloc;
    typedef typename Allocator<Type::Standard, int> std_alloc;

    PoolResult result;

#ifdef BOOST_MONOTONIC_TBB
    typedef Allocator<Type::Tbb, int> tbb_alloc;
    if (types.Includes(Type::Tbb))
    {
        SeedRand();
        boost::timer timer;
        for (size_t n = 0; n < count; ++n)
        {
            {
                fun.test(tbb_alloc(), length);
            }
        }
        result.tbb_elapsed = timer.elapsed();
    }
#endif

    if (types.Includes(Type::Monotonic))
    {
        SeedRand();
        auto_cpu_timer timer;
        for (size_t n = 0; n < count; ++n)
        {
            {
                fun.test(mono_alloc(), length);
            }
            boost::monotonic::reset_storage();
        }
        result.mono_elapsed = elapsed_s(timer);
    }

	// do it again for local storage if testing monotonic
    if (types.Includes(Type::Monotonic))
    {
        SeedRand();
        monotonic::local<my_local> storage;
        auto_cpu_timer timer;
        for (size_t n = 0; n < count; ++n)
        {
            {
                fun.test(monotonic::allocator<void, my_local>(), length);
            }
            storage.reset();
        }
        result.local_mono_elapsed = elapsed_s(timer);
    }

    if (types.Includes(Type::Standard))
    {
        SeedRand();
        auto_cpu_timer timer;
        for (size_t n = 0; n < count; ++n)
        {
            {
                fun.test(std_alloc(), length);
            }
        }
        result.std_elapsed = elapsed_s(timer);
    }

    cout << "." << flush;
    return result;
}

pair<int, int> random_pair()
{
    return make_pair(rand(), rand());
}

template <class Fun>
PoolResults run_tests(
    size_t count, size_t max_length, size_t num_iterations, const char *title, Fun fun, 
    Type types = Type::Standard | Type::Monotonic | Type::Tbb
    )
{
    cout << title << ": reps=" << count << ", len=" << max_length << ", steps=" << num_iterations << endl;
    PoolResults results;
    SeedRand();
    auto_cpu_timer timer;
    for (size_t length = 10; length < max_length; length += max_length/num_iterations)
    {
        size_t required = length;

        if (random_numbers.size() < required)
            generate_n(back_inserter(random_numbers), required - random_numbers.size(), rand);

        if (random_pairs.size() < required)
            generate_n(back_inserter(random_pairs), required - random_pairs.size(), random_pair);

        results[length] = run_test(count, length, fun, types);
    }

    cout << endl << "took " << elapsed_s(timer) << "s" << endl;

    return results;
}

template <class II>
typename std::iterator_traits<II>::value_type calc_mean(II first, II last, size_t num)
{
    return std::accumulate(first, last, typename std::iterator_traits<II>::value_type(0))*(1.0/num);
}

template <class II>
typename std::iterator_traits<II>::value_type calc_mean(II first, II last)
{
    if (first == last)
        throw std::range_error("calc_mean");

    return calc_mean(first, last, std::distance(first, last));
}

template <class II>
pair<typename iterator_traits<II>::value_type, typename iterator_traits<II>::value_type> standard_deviation_mean(II first, II last)
{
    typedef typename iterator_traits<II>::value_type Value;
    size_t length = distance(first, last);
    if (length == 0)
        return make_pair(Value(0), Value(0));

    Value mean = calc_mean(first, last, length);
    Value std_dev = 0;
    for (; first != last; ++first)
    {
        Value val = *first - mean;
        std_dev += val*val;
    }

    std_dev = sqrt(std_dev*(1./length));
    return make_pair(std_dev, mean);
}

template <class Cont>
pair<typename Cont::value_type, typename Cont::value_type> standard_deviation_mean(Cont const &cont)
{
    return standard_deviation_mean(begin(cont), end(cont));
}

void print_cumulative(vector<PoolResult> const &results)
{
    pair<PoolResult, PoolResult> dev_mean = standard_deviation_mean(results);
    size_t w = 10;
    cout << setw(w) << "scheme" << setw(w) << "mean" << setw(w) << "std-dev" << setw(w) << "min" << setw(w) << "max" << endl;
    cout << setw(w) << "fast" << setprecision(3) << setw(w) << dev_mean.second.fast_pool_elapsed << setw(w) << dev_mean.first.fast_pool_elapsed << setw(w) << result_min.fast_pool_elapsed << setw(w) << result_max.fast_pool_elapsed << endl;
    cout << setw(w) << "pool" << setprecision(3) << setw(w) << dev_mean.second.pool_elapsed << setw(w) << dev_mean.first.pool_elapsed << setw(w) << result_min.pool_elapsed << setw(w) << result_max.pool_elapsed << endl;
    cout << setw(w) << "std" << setprecision(3) << setw(w) << dev_mean.second.std_elapsed << setw(w) << dev_mean.first.std_elapsed << setw(w) << result_min.std_elapsed << setw(w) << result_max.std_elapsed << endl;
#ifdef BOOST_MONOTONIC_TBB
    cout << setw(w) << "tbb" << setprecision(3) << setw(w) << dev_mean.second.tbb_elapsed << setw(w) << dev_mean.first.tbb_elapsed << setw(w) << result_min.tbb_elapsed << setw(w) << result_max.tbb_elapsed << endl;
#endif
    cout << endl;
}

void print(PoolResults const &results)
{
    size_t w = 10;
    cout << setw(4) << "len" << setw(w) << "fast/m" << setw(w) << "pool/m" << setw(w) << "std/m" << setw(w) << "tbb/m" << endl;
    cout << setw(0) << "--------------------------------------------" << endl;
    vector<PoolResult> results_vec;
    for (const auto& iter : results)
    {
        PoolResult const &result = iter.second;
        cout << setw(4) << iter.first << setprecision(3) << setw(w);
        if (result.local_mono_elapsed == 0)
        {
            cout << setw(w) << "mono = 0s" << endl;
            continue;
        }
        PoolResult ratio = result;
        ratio *= (1. / result.local_mono_elapsed);
        ratio.local_mono_elapsed = 1;

        if (first_result)
        {
            result_min = result_max = ratio;
            first_result = false;
        }
        else
        {
            result_min.update_min(ratio);
            result_max.update_max(ratio);
        }
        cout << ratio.fast_pool_elapsed << setw(w) << ratio.pool_elapsed << setw(w) << ratio.std_elapsed << setw(w) << ratio.tbb_elapsed << endl;
        results_vec.push_back(ratio);
        cumulative.push_back(ratio);
    }

    cout << endl;
    print_cumulative(results_vec);
    cout << endl << endl;
}

void heading(const char *text, char star = '-')
{
    size_t len = 55;
    for (size_t n = 0; n < len; ++n)
        cout << star;

    cout << endl << "\t\t" << text << endl;
    for (size_t n = 0; n < len; ++n)
        cout << star;

    cout << endl;
}

template <class Storage>
void test_locals(Storage &storage, size_t count)
{
    if (count == 0)
        return;

    typedef typename Storage::template allocator<int>::type allocator;
    typedef typename Storage::template allocator<char>::type char_allocator;
    list<int, allocator > list;
    fill_n(back_inserter(list), 100, 42);
    typedef basic_string<char, char_traits<char>, char_allocator> String;
}

struct local_1 {};
struct local_2 {};

// TODO
//void test_locals_mono()
//{
//    monotonic::local<local_1> storage1;
//    //monotonic::local<local_2> storage2;
//    for (size_t n = 0; n < 10; ++n)
//    {
//        //test_locals(storage1, 4);
//        //test_locals(storage2, 2);
//    }
//    //cout << "test_locals: storage1 stack,heap =" << storage1.fixed_used() << ", " << storage1.heap_used() << endl;
//    //cout << "test_locals: storage2 stack,heap =" << storage2.fixed_used() << ", " << storage2.heap_used() << endl;
//}
//
void test_locals_std_impl(size_t count)
{
    if (count == 0)
        return;

    list<int> list;
    fill_n(back_inserter(list), 100, 42);
    string s = "foo";
    for (size_t n = 0; n < 100; ++n)
        s += "bar";

    vector<int> vec;
    vec.resize(1000);
    test_locals_std_impl(--count);
}

void test_locals_std()
{
    for (size_t n = 0; n < 10; ++n)
    {
        test_locals_std_impl(4);
    }
}

nanosecond_type elapsed_ns(auto_cpu_timer const& timer)
{
    auto const &elapsed = timer.elapsed();
    return elapsed.system + elapsed.user;
}

nanosecond_type elapsed_ms(auto_cpu_timer const& timer)
{
    return (nanosecond_type)(elapsed_ns(timer) / 1000.);
}

nanosecond_type elapsed_s(auto_cpu_timer const& timer)
{
    return (nanosecond_type)((elapsed_ms(timer)) / 1000.);
}

void test_locals(size_t count)
{
    auto_cpu_timer t0;
    for (size_t n = 0; n < count; ++n)
    {
        //test_locals_mono();
    }
    auto mono_elapsed = t0.elapsed();

    auto_cpu_timer t1;
    for (size_t n = 0; n < count; ++n)
    {
        test_locals_std();
    }
    auto std_elapsed = t1.elapsed();

    cout << "mono: " << elapsed_ms(t0) / 1000. << "ms" << endl;
    cout << "std: " << elapsed_ms(t1) / 1000. << "ms" << endl;
}

int main()
{
    try
    {
        cout << "results of running test at:" << endl;
        cout << "https://svn.boost.org/svn/boost/sandbox/monotonic/libs/monotonic/test/Tests.h" << endl << endl;

        auto_cpu_timer timer;
        Type test_map_vector_types;
        Type test_dupe_list_types;

        bool run_small = 1;
        bool run_medium = 1;
        bool run_large = 1;

        // small-size (~100 elements) containers
        if (run_small)
        {
			first_result = true;
            heading("SMALL");
            print(run_tests(1000, 100, 10, "string_cat", test_string_cat()));
            print(run_tests(5000, 100, 10, "list_string", test_list_string()));
            print(run_tests(5000, 100, 10, "list_create<int>", test_list_create<int>()));
            print(run_tests(2000, 100, 10, "list_sort<int>", test_list_sort<int>()));
            print(run_tests(150000, 100, 10, "vector_create<int>", test_vector_create()));
            print(run_tests(100000, 100, 10, "vector_sort<int>", test_vector_sort<int>()));
            print(run_tests(200000, 100, 10, "vector_dupe", test_vector_dupe()));
            print(run_tests(20000, 100, 10, "list_dupe", test_list_dupe()));
            print(run_tests(100000, 100, 10, "vector_accumulate", test_vector_accumulate()));
            //print(run_tests(50, 100, 10, "set_vector", test_set_vector()));
            print(run_tests(500, 100, 10, "map_vector<int>", test_map_vector<int>()));

            heading("SUMMARY", '*');
            print_cumulative(cumulative);
        }

        // medium-size (~1000 elements) containers
        if (run_medium)
        {
			first_result = true;
            heading("MEDIUM");
            print(run_tests(1000, 5000, 10, "list_create<int>", test_list_create<int>()));
            print(run_tests(1000, 5000, 10, "list_sort<int>", test_list_sort<int>()));
            print(run_tests(10000, 5000, 10, "vector_create<int>", test_vector_create()));
            print(run_tests(3000, 5000, 10, "vector_sort<int>", test_vector_sort<int>()));
            print(run_tests(30000, 5000, 10, "vector_dupe", test_vector_dupe()));
            print(run_tests(500, 5000, 10, "list_dupe", test_list_dupe(), test_dupe_list_types));
            print(run_tests(5000, 5000, 10, "vector_accumulate", test_vector_accumulate()));
            //print(run_tests(200, 200, 5, "set_vector", test_set_vector()));
            print(run_tests(50, 1000, 10, "map_vector<int>", test_map_vector<int>()));
            heading("SUMMARY", '*');
            print_cumulative(cumulative);
        }

        // large-size (~1000000 elements) containers
        if (run_large)
        {
			first_result = true;
            heading("LARGE");
            print(run_tests(10, 25000, 10, "list_create<int>", test_list_create<int>()));
            print(run_tests(10, 100000, 10, "list_sort<int>", test_list_sort<int>()));
            print(run_tests(2000, 100000, 10, "vector_create<int>", test_vector_create()));
            print(run_tests(500, 50000, 10, "vector_sort<int>", test_vector_sort<int>()));
            print(run_tests(500, 1000000, 10, "vector_dupe", test_vector_dupe()));
            print(run_tests(50, 10000, 10, "list_dupe", test_list_dupe(), test_dupe_list_types));
            print(run_tests(1000, 100000, 10, "vector_accumulate", test_vector_accumulate()));
            //print(run_tests(5, 500, 5, "set_vector", test_set_vector()));
            print(run_tests(20, 20000, 10, "map_vector<int>", test_map_vector<int>()));
        }

        heading("FINAL SUMMARY", '*');
        print_cumulative(cumulative);
        cout << endl << "took " << setprecision(3) << elapsed_s(timer)/60. << " minutes" << endl;
    }
    catch (exception &e)
    {
        cout << "exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}

