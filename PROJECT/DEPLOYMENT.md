# Netlify frontend + real Linux backend

Published on 29 September 2026:
https://processpay-linux-lab-jashwanth.netlify.app

Netlify project: `5cfa010a-8cab-41d6-9ac1-bffedb451de0`.
Deployment: `6abb2c101add5ab29f0bffb4`.

The frontend is hosted on Netlify. The real Linux backend runs in this
computer's Ubuntu WSL instance through a temporary Cloudflare tunnel:
https://compromise-reporting-contemporary-weapon.trycloudflare.com

Keep this computer on, connected, and both the backend and tunnel running.
Stopping either process or restarting the computer interrupts payment actions.
The Netlify frontend stays published. A new quick tunnel gets a different URL;
update PROCESSPAY_BACKEND_URL, rebuild and redeploy if the tunnel is replaced.
No simulated process data is used.

Public HTTPS tests passed for successful OTP, incorrect OTP, decline, real
PID/PPID relationships and cross-origin access. The published browser page also
completed a successful payment. See `live-validation.txt` for captured results.

## Linux backend

Run `make`, then run `hosted_server.py` as a dedicated unprivileged service:

```bash
ALLOWED_ORIGINS=https://YOUR-SITE.netlify.app HOST=127.0.0.1 PORT=8001 python3 hosted_server.py
```

Configure the server's HTTPS reverse proxy to forward the backend domain to
127.0.0.1:8001. Only the HTTPS proxy should be internet-facing. Preserve `Origin`
and `X-Process-Session` request headers. Add proxy request-rate and connection
limits appropriate to the server. `/health` returns JSON for health checks.
Exact service and proxy configuration will be generated after the server and
domain are known, to preserve existing services on that host.

Alternatively build the provided Dockerfile and publish its port on loopback,
with a read-only filesystem, memory/CPU/PID limits and dropped capabilities.
The image runs the backend and payment workers as UID 10001. An init process
should be enabled for the container. The Docker image has not yet been built
or tested on the target server.

Hosted visitors receive separate random session tokens held in sessionStorage.
Each can control only its own worker. The backend caps total active payment
applications at eight and retained sessions at 100. An unfinished payment is
declined after five minutes; idle sessions expire after 30 minutes. Refreshing
reconnects to the same session. Backend restarts clear in-memory sessions.
These are resource limits for a teaching app, not account authentication.

## Netlify

Use the project root as the base directory. `netlify.toml` sets:

- Build command: `node build-netlify.mjs`
- Publish directory: `netlify-dist`
- Environment variable: `PROCESSPAY_BACKEND_URL=https://YOUR-BACKEND-DOMAIN`

The build fails intentionally until a valid HTTPS backend origin is configured.
Only the HTML, CSS and JavaScript are published. The worker, Linux server,
source documents and local logs stay out of the public frontend directory.
The localhost `dist/config.js` remains unchanged; Netlify configuration is
generated in its separate build output.

## Validation

`make test` checks the C lifecycle. The hosted integration test uses a dedicated
test backend on port 8011:

```bash
ALLOWED_ORIGINS=http://localhost:8010 PORT=8011 PAYMENT_TIMEOUT=8 python3 hosted_server.py
# In another terminal:
python3 hosted_tests.py
```

Tests cover visitor isolation, success, decline, incorrect OTP, timeout cleanup,
real PID/PPID relationships, missing sessions, rejected origins and reaping.
After deployment, the public Netlify page still needs an end-to-end check
against the deployed backend and its HTTPS/CORS configuration.
