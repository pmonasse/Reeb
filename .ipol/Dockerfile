# use one of the images from this repository: https://github.com/centreborelli/ipol-docker-images/
FROM alpine

# install additional debian packages
COPY .ipol/packages.txt packages.txt
RUN apk add $(cat packages.txt) && rm packages.txt

# NO PYTHON FOR THIS DEMO
# copy the requirements.txt and install python packages
# COPY requirements.txt requirements.txt
# RUN pip3 install --no-cache-dir -r requirements.txt && rm requirements.txt

# copy the code to $bin
ENV bin /workdir/bin/
RUN mkdir -p $bin
WORKDIR $bin
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
RUN wget https://github.com/pmonasse/bspline_interpolation_c/archive/refs/tags/v1.3.tar.gz && tar xzf v1.3.tar.gz
RUN cmake -S bspline_interpolation_c-1.3/src -B build2 -DCMAKE_BUILD_TYPE=Release -DEXTRAPOLATE:BOOL=ON && cmake --build build2 && cp build2/bspline $bin

# the execution will happen in the folder /workdir/exec
# it will be created by IPOL

# some QoL tweaks
ENV PYTHONDONTWRITEBYTECODE 1
ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION python
ENV PATH $bin:$PATH

# $HOME is writable by the user `ipol`, but 
ENV HOME /home/ipol
# chmod 777 so that any user can use the HOME, in case the docker is run with -u 1001:1001
RUN adduser -D ipol -h $HOME && chmod -R 777 $HOME
USER ipol
