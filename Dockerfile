FROM emscripten/emsdk
RUN apt-get update && apt-get install cmake build-essential
RUN curl -fsSL https://deb.nodesource.com/setup_16.x | bash -
RUN apt-get install -y nodejs

USER root
WORKDIR /usr/src/
RUN mkdir spareduck
WORKDIR /usr/src/spareduck
COPY CMakeLists.txt ./
COPY build.sh ./
COPY src ./src
RUN bash build.sh
COPY test ./test
COPY test.sh ./
COPY www ./www
RUN cp build/spareduck.js ./www/ && mkdir -p ./www/dist && cp build/spareduck.wasm ./www/dist && cp build/spareduck.wasm ./www && cp ./www/index.html ./www/dist/
RUN echo $PATH
WORKDIR /usr/src/spareduck/www
RUN npm install
EXPOSE 8080
CMD ["npm", "run", "start"]