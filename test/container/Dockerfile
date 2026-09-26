FROM pgxn/pgxn-tools

# Major version of Postgres to put under test (18, 19, etc.)
ARG PG_MAJOR

# Install Postgres and create a test cluster at build time.
RUN NO_CLUSTER=1 pg-start "$PG_MAJOR" && \
    pg_createcluster "$PG_MAJOR" test -p 5432 -- -A trust

ENV PG_MAJOR=$PG_MAJOR

# Allow GNUMAKEFLAGS to be passed to sudo
RUN echo 'Defaults env_keep += "GNUMAKEFLAGS"' > /etc/sudoers.d/gnumakeflags && \
    chmod 440 /etc/sudoers.d/gnumakeflags

COPY run.sh /usr/local/bin/run.sh

CMD ["run.sh"]
