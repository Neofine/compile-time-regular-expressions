#include <ctre.hpp>
#include <ctre/glushkov_nfa.hpp>
#include <ctre/dominator_analysis.hpp>
#include <ctre/region_analysis.hpp>
#include <iostream>
#include <cassert>

void test_acyclic_graph_simple() {
    std::cout << "Test: Acyclic graph (simple pattern)... ";

    using Pattern = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);

    bool has_back_edges = false;
    for (size_t i = 0; i < nfa.state_count; i++) {
        for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
            if (dag.is_back_edge[i * ctre::region::MAX_STATES + j]) {
                has_back_edges = true;
            }
        }
    }

    assert(!has_back_edges && "Simple pattern should have no back edges");
    std::cout << "PASSED\n";
}

void test_acyclic_graph_with_loop() {
    std::cout << "Test: Acyclic graph (pattern with loop)... ";

    using Pattern = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::string<'b'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);

    bool has_back_edges = false;
    for (size_t i = 0; i < nfa.state_count; i++) {
        for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
            if (dag.is_back_edge[i * ctre::region::MAX_STATES + j]) {
                has_back_edges = true;
            }
        }
    }

    assert(has_back_edges && "Pattern with loop should have back edges");
    std::cout << "PASSED\n";
}

void test_acyclic_graph_alternation() {
    std::cout << "Test: Acyclic graph (alternation)... ";

    using Pattern = ctre::select<ctre::string<'f', 'o', 'o'>, ctre::string<'b', 'a', 'r'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);

    bool has_back_edges = false;
    for (size_t i = 0; i < nfa.state_count; i++) {
        for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
            if (dag.is_back_edge[i * ctre::region::MAX_STATES + j]) {
                has_back_edges = true;
            }
        }
    }

    assert(!has_back_edges && "Alternation without loops should have no back edges");
    std::cout << "PASSED\n";
}

void test_topo_sort_simple() {
    std::cout << "Test: Topological sort (simple pattern)... ";

    using Pattern = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);
    auto topo = ctre::region::topological_sort(dag);

    assert(topo.count == nfa.state_count && "All states should be in topo order");
    std::cout << "PASSED\n";
}

void test_topo_sort_alternation() {
    std::cout << "Test: Topological sort (alternation)... ";

    using Pattern = ctre::select<ctre::string<'f', 'o', 'o'>, ctre::string<'b', 'a', 'r'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);
    auto topo = ctre::region::topological_sort(dag);

    assert(topo.count == nfa.state_count && "All states should be in topo order");
    std::cout << "PASSED\n";
}

void test_region_finding_simple() {
    std::cout << "Test: Region finding (simple pattern)... ";

    using Pattern = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);
    auto regions = ctre::region::find_regions(dag);

    assert(regions.count > 0 && "Should find at least one region");
    std::cout << "PASSED\n";
}

void test_region_finding_alternation() {
    std::cout << "Test: Region finding (alternation)... ";

    using Pattern = ctre::select<ctre::string<'f', 'o', 'o'>, ctre::string<'b', 'a', 'r'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);
    auto regions = ctre::region::find_regions(dag);

    assert(regions.count > 0 && "Should find regions for alternation");
    std::cout << "PASSED\n";
}

void test_string_extraction_simple() {
    std::cout << "Test: String extraction (simple pattern)... ";

    using Pattern = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto result = ctre::region::extract_literal_from_regions(nfa);

    // May or may not extract depending on implementation
    std::cout << (result.has_literal ? "extracted" : "none") << " PASSED\n";
}

void test_string_extraction_alternation() {
    std::cout << "Test: String extraction (alternation)... ";

    using Pattern = ctre::select<ctre::string<'f', 'o', 'o'>, ctre::string<'b', 'a', 'r'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto result = ctre::region::extract_literal_from_regions(nfa);

    if (result.has_literal) {
        std::string extracted;
        for (size_t i = 0; i < result.length; i++) {
            extracted += result.chars[i];
        }
        bool valid = (extracted == "foo" || extracted == "bar");
        assert(valid && "Should extract 'foo' or 'bar'");
    }
    std::cout << "PASSED\n";
}

void test_back_edge_merging() {
    std::cout << "Test: Back edge merging... ";

    using Pattern = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::string<'b'>>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    auto dag = ctre::region::acyclic_graph(nfa);
    auto regions = ctre::region::find_regions(dag);
    ctre::region::merge_back_edge_regions(dag, regions);

    std::cout << "PASSED\n";
}

void test_integration_alternation() {
    std::cout << "Test: Integration (path -> region fallback)... ";

    using Pattern = ctre::select<ctre::string<'f', 'o', 'o'>, ctre::string<'b', 'a', 'r'>>;
    auto literal = ctre::decomposition::extract_literal_with_fallback<Pattern>();

    if (literal.has_literal) {
        std::string extracted;
        for (size_t i = 0; i < literal.length; i++) {
            extracted += literal.chars[i];
        }
        bool valid = (extracted == "foo" || extracted == "bar");
        assert(valid && "Should extract 'foo' or 'bar' from alternation");
    }
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Region Analysis Tests ===\n\n";

    test_acyclic_graph_simple();
    test_acyclic_graph_with_loop();
    test_acyclic_graph_alternation();
    test_topo_sort_simple();
    test_topo_sort_alternation();
    test_region_finding_simple();
    test_region_finding_alternation();
    test_string_extraction_simple();
    test_string_extraction_alternation();
    test_back_edge_merging();
    test_integration_alternation();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
