# TPredictX7 Indicator Server

Rust service for client presence and coordination.

Data flow:

`Client -> HTTPS :8779/token.json -> UDP :8778 -> Rust service -> users.json -> HTTPS :8779/users.json -> clients/server browser`

Default ports:

- UDP presence: `8778`
- HTTPS web: `8779`

## Config

The service reads environment variables and `.env` files.

Important variables:

- `APP_SHARED_TOKEN`: normal presence token.
- `APP_SECRET_KEY`: developer icon secret.
- `UDP_BIND`, `WEB_HOST`, `WEB_PORT`: bind addresses.
- `TLS_CERT_FILE`, `TLS_KEY_FILE`: HTTPS certificate files.

## Run

```bash
./src/clientindicatorsrv/run.sh start
./src/clientindicatorsrv/run.sh status
```
