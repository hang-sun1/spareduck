FROM emscripten/emsdk
RUN apt-get update && apt-get install cmake build-essential
RUN curl -fsSL https://deb.nodesource.com/setup_16.x | bash -
RUN apt-get install -y nodejs

USER root
WORKDIR /usr/src/
RUN mkdir spareduck
WORKDIR /usr/src/spareduck
COPY CMakeLists.txt ./
COPY src ./src
COPY www ./www
RUN rm build/ -rf && mkdir build
WORKDIR /usr/src/spareduck/build
RUN cmake -D CMAKE_C_COMPILER=emcc -D CMAKE_CXX_COMPILER=em++ .. && make
RUN mv spareduck.js ../www/ && mkdir -p ../www/dist && cp spareduck.wasm ../www/dist && mv spareduck.wasm ../www && mv ../www/index.html ../www/dist/

WORKDIR /usr/src/spareduck/www
RUN npm install
CMD ["npm", "run", "start"]
