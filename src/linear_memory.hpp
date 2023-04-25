#ifndef LINEAR_MEMORY_H
#define LINEAR_MEMORY_H

#include <iostream>

#include <math.h>
#include <string.h>

#include <vector>
#include <cassert>


template<typename T>
class linear_memory
{
    private:
        bool bool_manage_memory = false;
        // [2023/04/04]
        // NOTE: We added this variable so that this class can be used as wrapper to 
        //       already allocated memory.

    public:
        long length = -(1<<30);
        T * ptr = nullptr;

        int dim = 1;
        std::vector<long> vec_length {length};
        std::vector<long> vec_stride {1};
        // [2023/04/04]
        // NOTE: The above syntax initialize one-element vectors. The syntax can be
        //       confusing, as illustrated by the following snippet from cppreference:
        //       
        //       std::vector<int> b{3}; // creates a 1-element vector holding {3}
        //       std::vector<int> a(3); // creates a 3-element vector holding {0, 0, 0}
        // 
        //       std::vector<int> d{1, 2}; // creates a 2-element vector holding {1, 2}
        //       std::vector<int> c(1, 2); // creates a 1-element vector holding {2}
        //       
        //       We may want to change the syntax to 
        //                  std::vector<long> vec_length = {length};
        //                  std::vector<long> vec_stride = {1};
        //       to avoid confusion.
              

        std::string name = "undefined";
        // [2022/12/27]
        // NOTE: Give each linear_memory an std::string to store an optional name.
        //       This can be helpful for debugging or for ensuring that the intended 
        //       linear_memory is used.

        linear_memory(){}  
        // [2023/04/24]
        // NOTE: Empty constructor in case we need to declare a linear_memory before 
        //       knowing its size or only need it to be effective on certain processes. 
        //       The above provides a user-defined default constructor.

        void allocate_memory (long const L) 
        {
            if ( ptr != nullptr ) 
            { 
                printf( "%s %d: ptr is already allocated for %s.\n", 
                        __FILE__, __LINE__, this->name.c_str() ); fflush(stdout); exit(0); 
            }

            if ( L <= 0 ) 
            { 
                printf( "%s %d: L <= 0 for %s.\n", 
                        __FILE__, __LINE__, this->name.c_str() ); fflush(stdout); exit(0); 
            }

            if ( L * sizeof(T) >= 1024.*1024.*1024. * 2. )  // use fp to avoid integer overflow
            { 
                printf( "%s %d: requested size for %s is larger than 2GB; if this "
                        "is intended, change the limit in the above if statement.\n", 
                        __FILE__, __LINE__, this->name.c_str() ); fflush(stdout); exit(0); 
            }


            constexpr long alignment = 32;    // 32 bytes; 
            long byte_size = ( ( L*sizeof(T) + alignment - 1 ) / alignment ) * alignment;

            ptr = static_cast<T *>( aligned_alloc(alignment, byte_size) );
            if ( ptr == nullptr ) 
            { 
                printf( "%s %d: aligned_alloc returned a nullptr for %s.\n", 
                        __FILE__, __LINE__, this->name.c_str() ); fflush(stdout); exit(0); 
            }
            memset ( ptr, 0, byte_size );


            bool_manage_memory = true;
            this->length = L;
            this->vec_length.at(0) = L;
        }


        void attach_dimension ( std::vector<long> const & V_len ) 
        {
            vec_length = V_len;
            dim = V_len.size();

            long deduced_length = 1;
            for ( int i = 0; i < dim; i++ ) { deduced_length *= vec_length.at(i); }
            assert ( length == deduced_length && "Error: length does not match" );

            vec_stride = std::vector<long> (dim,1);  // size dim, all entries with value 1
            for ( int i = 0    ; i < dim; i++ )
            for ( int j = i + 1; j < dim; j++ ) 
                { vec_stride.at(i) *= vec_length.at(j); }

            // [2022/10/11]
            // NOTE: With a recent compiler, we could use the following sequence :
            //       "std::reverse ; std::exclusive_scan ; std::reverse" to deduce 
            //       the stride.
        }


        linear_memory (long const L)
            { allocate_memory (L); }


        linear_memory (long const L , std::vector<long> const & V_len)
        {
            allocate_memory (L); 
            attach_dimension ( V_len );
        }


        linear_memory (std::vector<long> const & V_len)
        {
            long L = 1;  for ( unsigned int i = 0; i < V_len.size(); i++ ) { L *= V_len.at(i); }

            allocate_memory (L); 
            attach_dimension ( V_len );
        }


        void copy ( linear_memory<T> const & v )
        {
            // NOTE: Syntactically, this copy member function should "work" for self assignment.
            //       Its behavior would be (arguably) correct. However, such self assignment is 
            //       an indication that some error may have occurred from the "application" side.
            //       We add the following (self copy) test to guard such misuse.
            if ( this == &v || this->ptr == v.ptr ) 
                { printf("%s %d error: self copy\n", __FILE__, __LINE__); fflush(stdout); exit(0); }
            
            if ( this->ptr == nullptr || v.ptr == nullptr ) 
                { printf("%s %d error: nullptr  \n", __FILE__, __LINE__); fflush(stdout); exit(0); }

            if ( length <= 0 || length != v.length ) 
                { printf("%s %d error: length   \n", __FILE__, __LINE__); fflush(stdout); exit(0); }

            for ( long i=0; i<length; i++ ) { ptr[i] = v.ptr[i]; }   // { this->at(i) = v.at(i); }

            // [2023/04/04]
            // NOTE: Decided to not copy "vec_length" here because the vector copied to can have 
            //       already been associated with a different interpretation of the data.
        }


        // copy ctor
        linear_memory( linear_memory<T> const & v ) : linear_memory ( v.length )
            { copy (v); }

        // copy assignment operator
        linear_memory<T> & operator=( linear_memory<T> const & v )
            { copy (v); return *this; }
        // [2023/04/04] NOTE: Do we actually want to return void?


        // move ctor
        linear_memory( linear_memory<T> && v ) noexcept
        {
            this->length = v.length;  this->ptr = v.ptr;  this->bool_manage_memory = true;

            v.length = 0;             v.ptr = nullptr;    v.bool_manage_memory = false;  

            // [2023/04/04]
            // NOTE: We NEED to set bool_manage_memory to true here. Otherwise, the memory
            //       won't be freed in dtor.
            // [2023/04/04]
            // NOTE: Setting v.bool_manage_memory to false is "optional" because freeing a
            //       nullptr is no-op, but good practice.
        }

        // // move assignment operator
        // linear_memory<T> & operator=( linear_memory<T> && v ) = delete;
        // // [2023/04/04]
        // // NOTE: If copy ctor or copy assignment operator is user-supplied, the above 
        // //       "delete" would be unnecessary according to "rules". It is expressive 
        // //       nonetheless. We may want to "delete" the move assignment operator to
        // //       prevent certain usage.


        // another move assignment operator
        linear_memory<T> & operator=( linear_memory<T> && v ) noexcept
        {
            // [2023/04/04]
            // NOTE: move assignment is only allowed when this container is empty; otherwise
            //       we would need to destroy resource during the move assignment, which I 
            //       feel should be discouraged.
            //       
            if ( ptr != nullptr || bool_manage_memory != false ) 
            { 
                printf("%s %d: move assignment is only allowed for the case when " 
                       "the moved-to vector is empty.\n", __FILE__, __LINE__); 
                fflush(stdout); exit(0); 
            }

            if ( this == &v || this->ptr == v.ptr ) 
                { printf("%s %d: self assignment\n", __FILE__, __LINE__); fflush(stdout); exit(0); }


            this->length = v.length;  this->ptr = v.ptr;  this->bool_manage_memory = true;

            v.length = 0;             v.ptr = nullptr;    v.bool_manage_memory = false;

            return *this;
        }
        // [2023/04/04] NOTE: Do we actually want to return void?


        // dtor
        // ~linear_memory() { if ( bool_manage_memory ) { free(ptr); } }
                
        // NOTE: The following version is intended for debugging and illustration.
        ~linear_memory() 
        { 
            if ( bool_manage_memory ) 
                { printf("About to release memory from %s\n\n", name.c_str()); free(ptr); } 
        }


        // assignment operators
        void operator= ( std::initializer_list<T> il ) 
        { 
            assert( static_cast<long unsigned>(length) == il.size() );

            long i = 0; for ( auto& e : il ) { this->at(i) = e; i++; }
        }

        void operator*= (T const &a) { for ( long i=0; i<length; i++ ) { ptr[i] *= a; } } 
                                                       // { this->operator()(i) *= a; }

        void operator/= (T const &a) { for ( long i=0; i<length; i++ ) { ptr[i] /= a; } }
                                                       // { this->operator()(i) /= a; }

        // member access
        T* data() { return ptr; }

        T& operator() (long const i) // read and write access
            { return ptr[i]; }


        T& at (long const i) // read and write access
        {
            if ( ptr == nullptr   ) 
                { printf("%s %d nullptr in at \n", __FILE__, __LINE__); fflush(stdout); exit(0); }
            if ( i<0 || i>=length ) 
                { printf("%s %d out of bound  \n", __FILE__, __LINE__); fflush(stdout); exit(0); }

            return ptr[i];
        }

        T& at (long const i) const // read access
        {
            if ( ptr == nullptr   ) 
                { printf("%s %d nullptr in at \n", __FILE__, __LINE__); fflush(stdout); exit(0); }
            if ( i<0 || i>=length ) 
                { printf("%s %d out of bound  \n", __FILE__, __LINE__); fflush(stdout); exit(0); }

            return ptr[i];
        }        

        
        T& operator() (std::vector<long> const & vec_ind) // read and write access
        {
            long ind = 0;
            for ( int i = 0; i < dim; i++ ) { ind += vec_ind.at(i) * vec_stride.at(i); }

            return ptr[ind];
        }


        T& at (std::vector<long> const & vec_ind) // read and write access
        {
            assert ( vec_ind.size() == vec_length.size() );
            assert ( (int) vec_ind.size() == dim );
            assert ( ptr != nullptr );

            long ind = 0;
            for ( int i = 0; i < dim; i++ ) { ind += vec_ind.at(i) * vec_stride.at(i); }

            assert ( ( 0 <= ind && ind < length ) && "Out of bound access" );
            return ptr[ind];
        }


        // ---- an overloaded .at() that lets us retrieve the entry by two "long"s to 
        //      mimic the calling convention for matrices. /* dim need to be 2 */
        //
        T& at ( long const & i0, long const & i1 ) // read and write access
        {
            assert ( dim == 2 && "This function is intended for a 2D array." );
            assert ( ptr != nullptr );

            long ind = i0 * vec_stride.at(0)
                     + i1 * vec_stride.at(1);

            assert ( ( 0 <= ind && ind < length ) && "Out of bound access" );
            return ptr[ind];
        }


        // ---- an overloaded .at() that lets us retrieve the entry by three "long"s to 
        //      mimic the calling convention for a 3D tensor. /* dim need to be 3 */
        //
        T& at ( long const & i0, long const & i1, long const & i2 ) // read and write access
        {
            assert ( dim == 3 && "This function is intended for a 3D array." );
            assert ( ptr != nullptr );

            long ind = i0 * vec_stride.at(0)
                     + i1 * vec_stride.at(1)
                     + i2 * vec_stride.at(2);

            assert ( ( 0 <= ind && ind < length ) && "Out of bound access" );
            return ptr[ind];
        }


        void set_constant(T const & a) { for ( long i = 0; i < length; i++ ) { ptr[i] = a; } }


        void ax_add_to ( T const &a , linear_memory<T> const &x ) 
        { 
            assert ( this->length == x.length );
            for ( long i=0; i<this->length; i++ ) { this->ptr[i] += a * x.ptr[i]; }
                                               // { this->at (i) += a * x    (i); }
        }


        void ax_by_assign ( T const &a , linear_memory<T> const &x ,
                            T const &b , linear_memory<T> const &y ) 
        { 
            assert ( this->length == x.length && this->length == y.length );
            for ( long i=0; i<this->length; i++ ) { this->ptr[i] = a * x.ptr[i] + b * y.ptr[i]; }
                                               // { this->at (i) = a * x    (i) + b * y    (i); }
        }


        auto L2_norm () const
            { return sqrt( L2_norm_square () ); }
        // NOTE: Be careful when T is integer type ( function signatures of sqrt can
        //       be found at https://en.cppreference.com/w/cpp/numeric/math/sqrt )


        T L2_norm_square () const
        {
            T norm_square = static_cast<T>(0);
            for ( long i=0; i<length; i++ ) { norm_square += this->ptr[i] * this->ptr[i]; }

            return norm_square;
        }


        void print (const char * format) { print (0, length, format); }

        void print (long const bgn, long const end, const char * format)
        {
            for ( long i=bgn; i<end; i++ ) { printf (format, this->ptr[i]); }
        }


        T* bgn () { return ptr;          }
        T* end () { return ptr + length; } // 1 past the last
        // [2023/04/24]
        // NOTE: bgn () and end () are added so that we can use the utility functions provided 
        //       in <algorithm> and others.
}; 
//class linear_memory


template<typename T>
T inner_product ( linear_memory<T> const & v1 , linear_memory<T> const & v2 )
{
    if ( v1.length <= 0 || v2.length <= 0 || v1.length != v2.length ) 
        { printf("%s %d Error: length\n", __FILE__, __LINE__); fflush(stdout); exit(0); }
    
    T inner_product = static_cast<T>(0);
    for ( long i = 0; i < v1.length; i++ ) { inner_product += v1.ptr[i] * v2.ptr[i]; }
    
    return inner_product;
}

#endif