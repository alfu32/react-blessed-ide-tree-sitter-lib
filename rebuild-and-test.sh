
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
sleep 2
time cmake --build build
sleep 2
cd src/python/
LD_PRELOAD=$(gcc -print-file-name=libasan.so) python3 -m unittest -v tokenizer_api_test.py
sleep 2
cd ../nodejs/
CFLAGS="-fsanitize=address -g" CXXFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address" \
npm run build
sleep 2
LD_PRELOAD=$(gcc -print-file-name=libasan.so) node ts_tokenizer_api.test.js
cd ../../