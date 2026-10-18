# Sign in with ChatGPT — Integration Blueprint (reviewed 2026-10-19)

**Current status:** a first Windows browser sign-in and Responses integration is implemented. See [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for exact behavior: memory-only tokens, re-sign-in on expiry, one saved registration, RS256/CNG verification; no persistent sessions or automatic refresh yet. Real account end-to-end verification remains pending. The complete target design below includes work not yet implemented. Re-check linked pages before coding. ChatGPT plan usage is an optional permission for eligible open-source, locally hosted clients; it is not a generic unrestricted free API. Paid/remotely hosted distribution requires contacting OpenAI.

## Official sources

- https://developers.openai.com/siwc/token-sharing-open-source
- https://developers.openai.com/siwc/token-sharing-open-source/sign-in
- https://developers.openai.com/siwc/token-sharing-open-source/profiles-and-sessions
- https://developers.openai.com/siwc/token-sharing-open-source/models-and-inference
- https://developers.openai.com/siwc/token-sharing-open-source/preview-limitations
- https://developers.openai.com/siwc/token-sharing-open-source/token-reference
- https://developers.openai.com/cookbook/articles/sign-in-with-chatgpt

## Flow / real identifiers

1. On first launch create/persist `ext_agent_host_id` **once per installation/host**. Supported example `urn:uuid:<persisted-UUIDv4>`; documented alternative: RFC9278 JWK thumbprint URI.
2. Open system browser. Start **loopback listener** on `127.0.0.1` *before* opening authorization. Use stable **path** `/auth/callback` (port may vary), no `localhost` substitution.
3. Each attempt creates fresh cryptographic `state`, OIDC `nonce`, PKCE verifier/challenge S256. `QOAuth2AuthorizationCodeFlow::setPkceMethod(S256)` is useful, but implement OpenAI-specific parameters and lifecycle correctly.
4. First registration: `client_id=dynamic_agent_client`, `agent_name_hint=MUTYVA`, persistent `ext_agent_host_id`, `response_type=code`, exact `redirect_uri`, `resource=https://api.openai.com/v1`, scopes: `openid profile email offline_access resource.invoke chatgpt.tokens.use.direct`.
5. Browser callback includes `code`, `state` and **issued** `client_id` (for registration). Verify `state` and save issued `client_id` paired to verified account. Never exchange code with `dynamic_agent_client`.
6. Token exchange: `POST https://auth.openai.com/api/accounts/oauth/token` with `grant_type=authorization_code`, issued `client_id`, `code`, `code_verifier`, exact `redirect_uri` and `resource=https://api.openai.com/v1`; no client secret required for this OSS public client.
7. Validate ID token **cryptographically** against official JWKS, check issuer, audience, expiry and matching nonce. Do not trust an unverified JWT payload. Confirm granted `chatgpt.tokens.use.direct` scope before AI requests.
8. Store issued `client_id`, tokens and account identity in protected storage, never plaintext in app JSON or log. Refresh with rotated refresh token through the token endpoint; serialize refresh operations.
9. For returning account reuse its issued `client_id` and persisted host ID. Account replacement must not silently overwrite other account sessions.
10. Sign out should revoke when supported, clear local credentials and allow the user to disconnect MUTYVA in ChatGPT Settings.

## Data model, minimally

```
ChatGPTAccount {
  verifiedSubject; issuedClientId; extAgentHostId;
  grantedScopes; selectedModelSlug;
  tokenStorageHandle; expiry; accountDisplayName;
}
```

No unencrypted token strings in SQLite, fixtures, project sources, screenshots or mutation history.

## Model selection and inference

Account-specific model list:

```
GET https://api.openai.com/v1/models
Authorization: Bearer <user OAuth access token>
```

Response is documented as `{ "models": [...] }`; select visible models (`visibility == "list"`), use returned `slug` as the `model`. Do NOT hardcode a model name as a permanent dependency. The UI can automatically choose an allowed model rather than burden normal users with selection.

Inference:

```http
POST https://api.openai.com/v1/responses
Authorization: Bearer <user OAuth access token>
Content-Type: application/json
```

Illustrative HTTP JSON body (select a real `model` slug from the account's list):

```json
{
  "model": "<AVAILABLE_MODEL_SLUG>",
  "store": false,
  "stream": true,
  "instructions": "Produce only a proposed MUTYVA mutation in the documented JSON format. Never request code execution.",
  "input": [
    { "role": "user", "content": "Добавь отдельную карточку влажности в City Pulse" }
  ]
}
```

Read SSE incrementally and accumulate the final assistant output until `response.completed`. On incomplete stream/error, discard proposal. The UI shows progress/cancel, may retry safely, and never applies partially received output. Parse the completed JSON and validate schema AND semantics. The structured-output features of individual account models/routes should be separately verified; do not assume every JSON formatting parameter is accepted.

**Preview restrictions:** HTTP requests require `store:false` and `stream:true`; include required history in each `input` array. Do not send `previous_response_id` or unsupported request fields such as `temperature`, `background`, `conversation`, or top-level system message items. Hosted file search, Code Interpreter, native computer use, image generation and hosted MCP/connectors are not available through this flow. See limitations documentation.

## Mutation request construction

`AiDesigner` sends only: current *redacted* app definition, available component schemas, allowed mutation operations, user request, selected binding metadata (not raw private records), and developer instructions. It should respond with `MutationProposal` JSON (see `specs/mutation.schema.json`).

Then: parse -> schema validation -> allowlist action validation -> check no ID collisions -> optional preview of data effects -> user confirmation -> atomic generation. **The assistant cannot directly execute any action.**

## Acceptance tests

- Auth succeeds for a consenting **eligible** Plus/Pro test account; without permission no inference.
- Cancel or denied consent leaves user in non-AI demo mode.
- Invalid state, nonce, signature, audience or scope causes hard failure, not silent downgrade.
- Refresh succeeds; revoked/expired tokens prompt re-sign-in. No auth loop.
- Model list is queried per account; does not depend on one hardcoded name.
- Stream interruptions never commit mutations; success requires `response.completed` and valid JSON.
- No tokens, cookies or personal account data in logs, app packages or repository.
- Actual flow works from system browser with permitted callback on each targeted device.

## Risk register

- Sign in with ChatGPT is a relatively new/preview integration; parameters and plan eligibility can change.
- Mobile loopback OAuth callback has platform behaviors to test early; do not promise iOS support until a physical-device end-to-end sign-in is verified.
- `store:false` means the client owns its own conversation context; the ChatGPT web-chat history is not available through the OAuth grant.
