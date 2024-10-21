#include <gtest/gtest.h>
#include <matplot/matplot.h>

TEST(MatplotTest, SimplePlot) {
    using namespace matplot;

    // Create a simple plot
    auto fig = figure(true);
    plot({1, 2, 3, 4, 5}, {1, 4, 9, 16, 25});
    title("Simple Plot");
    xlabel("X Axis");
    ylabel("Y Axis");

    // Save the plot to a file
    save("simple_plot.png");

    // Check if the file was created
    std::ifstream file("simple_plot.png");
    ASSERT_TRUE(file.good());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}