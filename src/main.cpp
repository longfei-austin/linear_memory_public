#include <numeric>

#include "linear_memory.hpp"

int main(int argc, char* argv[])
{   
    {
        linear_memory<int> v (10);
        std::iota(v.bgn(), v.end(), 0);
        v.print("%4d\n");
        v.name = "v";
    }

    {
        linear_memory<int> v (10);
        std::iota(v.bgn(), v.end(), 0);
        v.attach_dimension( {2,5} );

        for ( int i=0; i<v.vec_length.at(0); i++ )
        {
            for ( int j=0; j<v.vec_length.at(1); j++ )
                { printf("%4d ", v.at(i,j)); }
            printf("\n");    
        }
        
        printf(" %d %d element: %4d\n", 1, 1, v.at(1,1));
        v.name = "v";
    }

    {
        linear_memory<int> v ( {5,2} );
        std::iota(v.bgn(), v.end(), 0);

        for ( int i=0; i<v.vec_length.at(0); i++ )
        {
            for ( int j=0; j<v.vec_length.at(1); j++ )
                { printf("%4d ", v.at(i,j)); }
            printf("\n");    
        }
        
        printf(" %d %d element: %4d\n", 1, 1, v.at(1,1));
        v.name = "v";
    }

    {
        linear_memory<int> v1 (10);
        std::iota(v1.bgn(), v1.end(), 0);
        
        linear_memory<int> v2 ( {5,2} );
        // v2.copy(v1);
        v2 = v1;

        printf(" inner product: %4d\n", inner_product(v1,v2));
        printf(" norm: %16.15e\n", v1.L2_norm());

        v2.ax_add_to(-1, v1);  // subtract v1 -> 0

        v2.print("%4d\n");

        v1.name = "v1";
        v2.name = "v2";
    }





    return 0;
}
