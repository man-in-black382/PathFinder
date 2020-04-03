#include <numeric>

#include "Assert.hpp"

namespace Foundation
{
    namespace Halton
    {

        template <uint32_t Dimensionality>
        std::array<float, Dimensionality> Element(uint32_t elementIndex, std::optional<std::array<uint32_t, Dimensionality>> customBases)
        {
            uint32_t d = 0;
            uint32_t j = 0;

            std::array<float, Dimensionality> primeInv;
            std::array<float, Dimensionality> r;
            std::array<uint32_t, Dimensionality> t;

            r.fill(0.0);
            t.fill(elementIndex);

            //
            //  Carry out the computation.
            //
            for (j = 0; j < Dimensionality; j++)
            {
                float base = customBases ? (float)customBases.value().at(j) : (float)Prime(j + 1);
                primeInv[j] = (1.0 / base);
            }

            while (0.0 < std::accumulate(t.begin(), t.end(), 0.0))
            {
                for (j = 0; j < Dimensionality; j++)
                {
                    uint32_t base = customBases ? customBases.value().at(j) : Prime(j + 1);

                    d = (t[j] % base);
                    r[j] = r[j] + (float)(d) * primeInv[j];
                    primeInv[j] = primeInv[j] / (float)base;
                    t[j] = (t[j] / base);
                }
            }

            return r;
        }

        template <uint32_t Dimensionality>
        std::vector<std::array<float, Dimensionality>> Sequence(uint32_t elementStartIndex, uint32_t elementEndIndex)
        {
            assert_format(elementStartIndex <= elementEndIndex, "End index must be greater or equal to start index");

            uint32_t d = 0;
            uint32_t i = elementStartIndex;
            uint32_t j = 0;
            uint32_t k = 0;
            uint32_t n = elementEndIndex - elementStartIndex + 1;

            std::array<float, Dimensionality> primeInv;
            std::vector<std::array<float, Dimensionality>> r;
            std::array<uint32_t, Dimensionality> t;

            r.resize(n);

            for (auto& innerArray : r)
            {
                innerArray.fill(0.0);
            }

            for (k = 0; k < n; k++)
            {
                t.fill(i);

                //
                //  Carry out the computation.
                //
                for (j = 0; j < Dimensionality; j++)
                {
                    primeInv[j] = 1.0 / (float)(Prime(j + 1));
                }

                while (0 < std::accumulate(t.begin(), t.end(), 0.0))
                {
                    for (j = 0; j < Dimensionality; j++)
                    {
                        d = (t[j] % Prime(j + 1));
                        r[k][j] = r[k][j] + (float)(d)*primeInv[j];
                        primeInv[j] = primeInv[j] / (float)(Prime(j + 1));
                        t[j] = (t[j] / Prime(j + 1));
                    }
                }

                ++i;
            }

            return r;
        }

    }
}

