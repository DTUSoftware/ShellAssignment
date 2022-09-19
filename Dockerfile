FROM gcc:latest
COPY . /UCLI
WORKDIR /UCLI/
RUN gcc -o UCLI ucli.c
CMD ["./UCLI"]