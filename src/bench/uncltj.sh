BIN_FOLDER=/home/agomez/data/bench-cltj/cltj/build/bench
DATASET=/home/agomez/data/bench-cltj/data/wikidata.nt.enumerated
DATASET_80=/home/agomez/data/bench-cltj/data/wikidata-80.nt.enumerated
DATASET_RDF=/home/agomez/data/bench-cltj/data/wikidata.nt.rdf
QUERIES=/home/agomez/data/bench-cltj/data/Queries-bgps-limit1000.txt
UPDATES=/home/agomez/data/bench-cltj/data/updates-80.txt
OUTPUT_FOLDER=/home/agomez/data/bench-cltj/output


for i in 1 2 3
do
  $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 1000 star > $OUTPUT_FOLDER/uncltj-star-1000.txt
done

for i in 1 2 3
do
  $BIN_FOLDER/bench-query-uncltj $DATASET.uncltj $QUERIES 1000 normal > $OUTPUT_FOLDER/uncltj-normal-1000.txt
done
