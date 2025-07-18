DATASET=/home/agomez/data/SNAP/soc-LiveJournal1-norm/soc-LiveJournal1-norm.nt
QUERIES=/home/agomez/data/SNAP/soc-LiveJournal1-norm/queries/
OUTPUT_FOLDER=/home/agomez/data/bench-cltj/output/lj
BIN_FOLDER=/home/agomez/data/bench-cltj/cltj/build/bench

echo "Building indices..."
$BIN_FOLDER/build-cltj $DATASET > $OUTPUT_FOLDER/cltj-build-lj.txt
$BIN_FOLDER/build-xcltj $DATASET > $OUTPUT_FOLDER/xcltj-build-lj.txt
$BIN_FOLDER/build-uncltj $DATASET > $OUTPUT_FOLDER/uncltj-build-lj.txt
echo "Done building indices."
echo "3.1 Running CLTJ..."
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/1-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-1-tree.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/2-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-2-tree.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/2-comb.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-2-comb.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/3-path.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-3-path.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/4-path.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-4-path.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/2-3-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-2-3-lollipop.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/3-4-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-3-4-lollipop.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/cliques.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-cliques.txt
$BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES/cycles.tsv 0 star 1800 > $OUTPUT_FOLDER/cltj-cycles.txt
echo "3.2 Running xCLTJ..."
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/1-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-1-tree.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/2-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-2-tree.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/2-comb.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-2-comb.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/3-path.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-3-path.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/4-path.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-4-path.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/2-3-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-2-3-lollipop.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/3-4-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-3-4-lollipop.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/cliques.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-cliques.txt
$BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES/cycles.tsv 0 star 1800 > $OUTPUT_FOLDER/xcltj-cycles.txt
echo "3.3 Running UnCLTJ..."
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/1-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-1-tree.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/2-tree.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-2-tree.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/2-comb.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-2-comb.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/3-path.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-3-path.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/4-path.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-4-path.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/2-3-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-2-3-lollipop.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/3-4-lollipop.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-3-4-lollipop.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/cliques.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-cliques.txt
$BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES/cycles.tsv 0 star 1800 > $OUTPUT_FOLDER/uncltj-cycles.txt
echo "Done running benchmark."
