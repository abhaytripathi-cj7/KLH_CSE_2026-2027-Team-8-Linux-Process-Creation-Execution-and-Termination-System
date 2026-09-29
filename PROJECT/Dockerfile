FROM gcc:14-bookworm AS build
WORKDIR /src
COPY payment.c Makefile ./
RUN make

FROM python:3.12-slim-bookworm
WORKDIR /app
COPY --from=build /src/payment /app/payment
COPY server.py hosted_server.py ./
RUN useradd --uid 10001 --no-create-home app
USER 10001
ENV HOST=0.0.0.0 PORT=8001 PYTHONUNBUFFERED=1
EXPOSE 8001
CMD ["python3", "hosted_server.py"]
