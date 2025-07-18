BIN_FOLDER=/mnt/data/cltj/build/bench
DATASET=/mnt/data/dataset/wikidata-enumerated.dat
DATASET_80=/mnt/data/dataset/wikidata-enumerated-80.dat
DATASET_RDF=/mnt/data/dataset/wikidata-enumerated.dat
QUERIES=/mnt/data/dataset/wikidata-enumerated-queries.dat
UPDATES=/mnt/data/dataset/wikidata-enumerated-updates.dat
INDELS=/mnt/data/dataset/wikidata-enumerated-indels.dat
OUTPUT_FOLDER=/mnt/data/cltj/build/bench

RUN_STATIC=1
RUN_DYNAMIC=1
RUN_RDF=1

if [ $RUN_STATIC -eq 1 ]; then
   echo "Building static indices..."
   $BIN_FOLDER/build-cltj $DATASET > $OUTPUT_FOLDER/cltj-build.txt
   $BIN_FOLDER/build-xcltj $DATASET > $OUTPUT_FOLDER/xcltj-build.txt
   $BIN_FOLDER/build-uncltj $DATASET > $OUTPUT_FOLDER/uncltj-build.txt
   echo "Done building static indices."
   echo "1. Running static benchmarks with limit=1000..."
   echo "1.1 Running CLTJ..."
   $BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES 1000 star > $OUTPUT_FOLDER/cltj-star-1000.txt
   $BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES 1000 normal > $OUTPUT_FOLDER/cltj-normal-1000.txt
   $BIN_FOLDER/bench-query-cltj-global $DATASET.cltj $QUERIES 1000 star > $OUTPUT_FOLDER/cltj-global-star-1000.txt
   $BIN_FOLDER/bench-query-cltj-global $DATASET.cltj $QUERIES 1000 normal > $OUTPUT_FOLDER/cltj-global-normal-1000.txt
   echo "1.2 Running xCLTJ..."
   $BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES 1000 star > $OUTPUT_FOLDER/xcltj-star-1000.txt
   $BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES 1000 normal > $OUTPUT_FOLDER/xcltj-normal-1000.txt
   $BIN_FOLDER/bench-query-xcltj-global $DATASET.xcltj $QUERIES 1000 star > $OUTPUT_FOLDER/xcltj-global-star-1000.txt
   $BIN_FOLDER/bench-query-xcltj-global $DATASET.xcltj $QUERIES 1000 normal > $OUTPUT_FOLDER/xcltj-global-normal-1000.txt
   echo "1.3 Running UnCLTJ..."
   $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 1000 star > $OUTPUT_FOLDER/uncltj-star-1000.txt
   $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 1000 normal > $OUTPUT_FOLDER/uncltj-normal-1000.txt
   $BIN_FOLDER/bench-query-uncltj-global $DATASET.uncltj $QUERIES 1000 star > $OUTPUT_FOLDER/uncltj-global-star-1000.txt
   $BIN_FOLDER/bench-query-uncltj-global $DATASET.uncltj $QUERIES 1000 normal > $OUTPUT_FOLDER/uncltj-global-normal-1000.txt
   echo "Done running static benchmarks with limit=1000."

   echo "2. Running static benchmarks with limit=0..."
   echo "2.1 Running CLTJ..."
   $BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES 0 star > $OUTPUT_FOLDER/cltj-star-0.txt
   $BIN_FOLDER/bench-query-cltj $DATASET.cltj $QUERIES 0 normal > $OUTPUT_FOLDER/cltj-normal-0.txt
   $BIN_FOLDER/bench-query-cltj-global $DATASET.cltj $QUERIES 0 star > $OUTPUT_FOLDER/cltj-global-star-0.txt
   $BIN_FOLDER/bench-query-cltj-global $DATASET.cltj $QUERIES 0 normal > $OUTPUT_FOLDER/cltj-global-normal-0.txt
   echo "2.2 Running xCLTJ..."
   $BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES 0 star > $OUTPUT_FOLDER/xcltj-star-0.txt
   $BIN_FOLDER/bench-query-xcltj $DATASET.xcltj $QUERIES 0 normal > $OUTPUT_FOLDER/xcltj-normal-0.txt
   $BIN_FOLDER/bench-query-xcltj-global $DATASET.xcltj $QUERIES 0 star > $OUTPUT_FOLDER/xcltj-global-star-0.txt
   $BIN_FOLDER/bench-query-xcltj-global $DATASET.xcltj $QUERIES 0 normal > $OUTPUT_FOLDER/xcltj-global-normal-0.txt
   echo "2.3 Running UnCLTJ..."
   $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 0 star > $OUTPUT_FOLDER/uncltj-star-0.txt
   $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 0 normal > $OUTPUT_FOLDER/uncltj-normal-0.txt
   $BIN_FOLDER/bench-query-uncltj-global $DATASET.uncltj $QUERIES 0 star > $OUTPUT_FOLDER/uncltj-global-star-0.txt
   $BIN_FOLDER/bench-query-uncltj-global $DATASET.uncltj $QUERIES 0 normal > $OUTPUT_FOLDER/uncltj-global-normal-0.txt
   echo "Done running static benchmarks with limit=0."
else
    echo "Skipping static benchmarks..."
fi

if [ $RUN_DYNAMIC -eq 1 ]; then
  echo "3. Running dynamic benchmarks with limit=0..."
  echo "Building dynamic indices..."
  #$BIN_FOLDER/build-cltj-dyn $DATASET_80 > $OUTPUT_FOLDER/cltj-dyn-build.txt
  $BIN_FOLDER/build-xcltj-dyn $DATASET_80 > $OUTPUT_FOLDER/xcltj-dyn-build.txt
  echo "Done building dynamic indices."
  #echo "3.1 Running CLTJ..."
  #for ratio in 0.001 0.01 0.1 1 10 100 1000
  #do
  #    $BIN_FOLDER/bench-update-cltj $DATASET_80.cltj-dyn $QUERIES $UPDATES $ratio 0 star > $OUTPUT_FOLDER/cltj-dyn-star-$ratio-0.txt
  #done
  #$BIN_FOLDER/bench-indels-cltj $DATASET_80.cltj-dyn $INDELS > $OUTPUT_FOLDER/cltj-dyn-indels.txt
  echo "3.2 Running xCLTJ..."
  for ratio in 0.001 0.01 0.1 1 10 100 1000
  do
      $BIN_FOLDER/bench-update-xcltj $DATASET_80.xcltj-dyn $QUERIES $UPDATES $ratio 0 star > $OUTPUT_FOLDER/xcltj-dyn-star-$ratio-0.txt
  done
  $BIN_FOLDER/bench-indels-xcltj $DATASET_80.xcltj-dyn $INDELS > $OUTPUT_FOLDER/xcltj-dyn-indels.txt
  echo "Done running dynamic benchmarks with limit=0."
else
    echo "Skipping dynamic benchmarks..."
fi

if [ $RUN_RDF -eq 1 ]; then
  echo "4. Running RDF benchmarks with limit=1000..."
  echo "Building RDF indices..."
  $BIN_FOLDER/build-cltj-rdf $DATASET_RDF > $OUTPUT_FOLDER/cltj-rdf-build.txt
  $BIN_FOLDER/build-xcltj-rdf $DATASET_RDF > $OUTPUT_FOLDER/xcltj-rdf-build.txt
  echo "Done building RDF indices."
  echo "4.1 Running CLTJ..."
  $BIN_FOLDER/bench-query-cltj-rdf $DATASET_RDF $QUERIES 1000 star > $OUTPUT_FOLDER/cltj-rdf-star-1000.txt
  echo "4.2 Running xCLTJ..."
  $BIN_FOLDER/bench-query-xcltj-rdf $DATASET_RDF $QUERIES 1000 star > $OUTPUT_FOLDER/xcltj-rdf-star-1000.txt
  echo "Done running RDF benchmarks with limit=1000."
else
    echo "Skipping RDF benchmarks..."
fi

