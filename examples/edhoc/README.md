# EDHOC Examples

This directory contains simple examples demonstrating how to use the EDHOC (Ephemeral Diffie-Hellman Over COSE) implementation in Contiki-NG.

EDHOC is a lightweight authenticated key exchange protocol described in [RFC 9528](https://tools.ietf.org/rfc/rfc9528.txt).

## Directory Structure

- `edhoc-client/` - Simple EDHOC client example (Initiator)
- `edhoc-server/` - Simple EDHOC server example (Responder)

## Quick Start

### Building the Examples

#### EDHOC Server
```bash
cd examples/edhoc/edhoc-server
make TARGET=native
```

#### EDHOC Client
```bash
cd examples/edhoc/edhoc-client
make TARGET=native
```

### Running the Examples

#### Native Platform

1. **Start the server:**
   ```bash
   cd examples/edhoc/edhoc-server
   ./edhoc-server-example.native
   ```

2. **Start the client (in another terminal):**
   ```bash
   cd examples/edhoc/edhoc-client
   ./edhoc-client-example.native
   ```

#### Cooja Simulation

Run the interactive demo simulation:
```bash
cd examples/edhoc
make COOJA_ARGS="--no-gui" edhoc-demo.csc
```

For interactive mode (with GUI):
```bash
cd examples/edhoc
make edhoc-demo.csc
```

## Configuration

### Server Configuration
Edit `edhoc-server/project-conf.h` to customize:
- Server identity: `EDHOC_AUTH_KID` (or `EDHOC_AUTH_SUBJECT_NAME`)
- Logging levels
- CoAP and network parameters

### Client Configuration
Edit `edhoc-client/project-conf.h` to customize:
- Server endpoint: `EDHOC_CONF_SERVER_EP`
- Logging levels
- CoAP and network parameters

## EDHOC Protocol Flow

1. **Client (Initiator)** sends MSG_1 to `/.well-known/edhoc`
2. **Server (Responder)** responds with MSG_2
3. **Client** sends MSG_3 to complete the handshake
4. **Secure communication** can now proceed using derived keys

## Advanced Usage

For more comprehensive testing and advanced features, see the test suite in:
- `tests/21-security-protocols/`

The test suite includes:
- Multi-method support (Method 0, Method 3)
- Comprehensive error handling
- Network simulation with Cooja
- Performance benchmarks

## Supported Platforms

- Native (Linux/macOS) - for development and testing
- CC2538DK - Texas Instruments platform
- Zoul - IoT platform
- Other Contiki-NG supported platforms

## Security Considerations

These examples use default keys and configuration suitable for testing. For production use:

1. **Generate unique keys** for each device
2. **Implement proper key storage** (secure elements, etc.)
3. **Configure appropriate cipher suites** for your security requirements
4. **Enable proper authentication** and authorization mechanisms

## Troubleshooting

### Common Issues

1. **Network connectivity**: Ensure both client and server can reach each other
2. **Key mismatch**: Verify both sides use compatible keys and cipher suites
3. **CoAP issues**: Check CoAP configuration and endpoint addresses

### Debug Logging

Enable debug logging by modifying the log levels in `project-conf.h`:
```c
#define LOG_CONF_LEVEL_EDHOC        LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_COAP         LOG_LEVEL_DBG
```

## Further Reading

- [EDHOC RFC 9528](https://tools.ietf.org/rfc/rfc9528.txt)
- [COSE RFC 8152](https://tools.ietf.org/rfc/rfc8152.txt)
- [Contiki-NG Documentation](https://docs.contiki-ng.org/)
