# Windows signing

This preview is unsigned until a real certificate is connected. The release-feed signature verifies downloaded bytes and does not identify the publisher to Windows.

Import your certificate and private key into the current user's Personal certificate store. Install SignTool from the Windows SDK. Set `BLUECLOUD_SIGN_WINDOWS=1` and `BLUECLOUD_CERT_THUMBPRINT` to the certificate's 40-character thumbprint. Optionally set `CONEIC_SIGNTOOL` to SignTool's absolute path and `BLUECLOUD_TIMESTAMP_URL` to your RFC 3161 timestamp service.

Run the normal Windows packaging script. It signs the application launchers before embedding them and signs and verifies the final installer. A missing certificate or failed signature stops packaging. Build metadata and media-engine preservation checks must run before signing; generate hashes and publish only after signing finishes.

Keep certificates, private keys and passwords outside the source checkout and release assets. A private signing key is never part of a source archive.
