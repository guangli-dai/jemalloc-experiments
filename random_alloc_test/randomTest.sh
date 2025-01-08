#!/bin/bash
# Turn off turbos and chef
sudo dynamo "$(hostname)" turnOffTurbo
sudo dynamo "$(hostname)" checkTurboStatus
sudo /usr/facebook/ops/scripts/chef/stop_chef_temporarily -r "Performance testing with turbo disabled" -t 12

INPUT_FILE="ART_full_sim.log"
RUNNING_COMMAND="
MALLOC_CONF=\"stats_print:true,dirty_decay_ms:5000,muzzy_decay_ms:0,background_thread:true\" ./multiThreadRandom 10 200 5 ${INPUT_FILE}"

echo "Testing on dev"
cd ~/gdai-jemalloc/jemalloc
git checkout dev
./autogen.sh --enable-prof && make -j64 && sudo make install

cd ~/gdai-jemalloc/jemalloc-experiments/random_alloc_test
g++ multiThreadRandomTest.cpp -o multiThreadRandom `jemalloc-config --libdir`/libjemalloc.a `jemalloc-config --libs` -O3
export LD_PRELOAD=/usr/local/lib/libjemalloc.so
eval ${RUNNING_COMMAND}

echo "Testing on the changes"
cd ~/gdai-jemalloc/jemalloc
#git checkout hpa_debug
git checkout hpa_test_2
./autogen.sh --enable-prof --enable-limit-usize-gap && make -j64 && sudo make install

cd ~/gdai-jemalloc/jemalloc-experiments/random_alloc_test
g++ multiThreadRandomTest.cpp -o multiThreadRandom `jemalloc-config --libdir`/libjemalloc.a `jemalloc-config --libs` -O3
export LD_PRELOAD=/usr/local/lib/libjemalloc.so
eval ${RUNNING_COMMAND}

