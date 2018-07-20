#include <cmath>
#include <cstdint>
#include <Sequence/VariantMatrix.hpp>
#include <Sequence/VariantMatrixViews.hpp>
#include <Sequence/summstats/nsl.hpp>

namespace
{
    std::int64_t
    get_left(const Sequence::ConstColView& sample_i,
             const Sequence::ConstColView& sample_j, const std::size_t core)
    {
        std::int64_t left = static_cast<std::int64_t>(core) - 1;
        std::size_t left_index;
        do
            {
                left_index = static_cast<std::size_t>(left);
                if (sample_i[left_index] >= 0 && sample_j[left_index] >= 0
                    && sample_i[left_index] != sample_j[left_index])
                    {
                        break;
                    }
            }
        while (--left >= 0);
        return left;
    }

    std::int64_t
    get_right(const Sequence::ConstColView& sample_i,
              const Sequence::ConstColView& sample_j, const std::size_t core,
              const std::int64_t nsam)
    {
        std::int64_t right = static_cast<std::int64_t>(core) + 1;
        std::size_t right_index;
        do
            {
                right_index = static_cast<std::size_t>(right);
                if (sample_i[right_index] >= 0 && sample_j[right_index] >= 0
                    && sample_i[right_index] != sample_j[right_index])
                    {
                        break;
                    }
            }
        while (++right < nsam);
        return right;
    }

    void
    update_counts(double nsl_values[2], double ihs_values[2], int counts[2],
                  const std::size_t nsam, const std::vector<double>& positions,
                  const std::size_t index, const std::int64_t left,
                  const std::int64_t right)
    {
        if (left >= 0 && static_cast<std::size_t>(right) < nsam)
            //Then there are SNPs differentiating
            //i and j within the region
            {
                nsl_values[index] += static_cast<double>(right - left);
                //TODO: check if we need to add one?
                ihs_values[index]
                    += positions[static_cast<std::size_t>(left)]
                       - positions[static_cast<std::size_t>(right)];
                counts[index]++;
            }
    }
} // namespace

namespace Sequence
{
    nSL
    nsl(const VariantMatrix& m, const std::size_t core,
        const std::int8_t refstate)
    {
        auto core_view = get_ConstRowView(m, core);
        // Keep track of distances from core site
        // for nsl and ihs separately
        double nsl_values[2] = { 0, 0 };
        double ihs_values[2] = { 0, 0 };
        //Count sample size for non-ref and ref alleles at core site contributing to nSL
        int counts[2] = { 0, 0 };
        for (std::size_t i = 0; i < m.nsam - 1; ++i)
            {
                auto sample_i = get_ConstColView(m, i);
                for (std::size_t j = i + 1; j < m.nsam; ++j)
                    {
                        auto sample_j = get_ConstColView(m, j);
                        //Find where samples i and j differ
                        auto left = get_left(sample_i, sample_j, core);
                        auto right
                            = get_right(sample_i, sample_j, core,
                                        static_cast<std::int64_t>(m.nsam));
                        update_counts(
                            nsl_values, ihs_values, counts, m.nsam,
                            m.positions,
                            static_cast<std::size_t>(core_view[i] == refstate),
                            left, right);
                    }
            }
        double nSL_num = nsl_values[0] / static_cast<double>(counts[0]);
        double nSL_den = nsl_values[1] / static_cast<double>(counts[1]);
        double iHS_num = nsl_values[0] / static_cast<double>(counts[0]);
        double iHS_den = nsl_values[1] / static_cast<double>(counts[1]);
        return nSL{ //TODO fix core count
                    std::log(nSL_num) - std::log(nSL_den),
                    std::log(iHS_num) - std::log(iHS_den), 1
        };
    }
} // namespace Sequence
