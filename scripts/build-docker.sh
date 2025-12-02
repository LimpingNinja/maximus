#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Build Docker image for Maximus BBS
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VERSION="${1:-dev}"
IMAGE_NAME="${2:-maximus-bbs}"
PUSH="${3:-false}"

cd "$PROJECT_ROOT"

echo "=== Building Maximus BBS Docker Image ==="
echo "Version: $VERSION"
echo "Image: $IMAGE_NAME"
echo ""

# Create Dockerfile if it doesn't exist
DOCKERFILE="$PROJECT_ROOT/Dockerfile"
if [ ! -f "$DOCKERFILE" ]; then
    echo "Creating Dockerfile..."
    cat > "$DOCKERFILE" << 'DOCKERFILE_CONTENT'
# Maximus BBS Docker Image
# Multi-stage build for minimal runtime image

# =============================================================================
# Build Stage
# =============================================================================
FROM debian:bookworm AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    libncurses-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /src
COPY . .

# Build
RUN make clean 2>/dev/null || true && make all

# =============================================================================
# Runtime Stage
# =============================================================================
FROM debian:bookworm-slim

LABEL maintainer="Kevin Morgan (Limping Ninja)" \
      description="Maximus BBS - Classic Bulletin Board System" \
      url="https://github.com/LimpingNinja/maximus"

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libncurses6 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -s /bin/bash bbs

# Copy built files from builder
COPY --from=builder /src/build /opt/maximus

# Set permissions
RUN chown -R bbs:bbs /opt/maximus

# Create volumes for persistent data
VOLUME ["/opt/maximus/etc", "/opt/maximus/var"]

# Expose telnet port
EXPOSE 2323

# Set working directory
WORKDIR /opt/maximus

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
    CMD nc -z localhost 2323 || exit 1

# Entry point script
COPY scripts/docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

USER bbs
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["maxtel", "-p", "2323", "-n", "4"]
DOCKERFILE_CONTENT
    echo "Dockerfile created."
fi

# Create entrypoint script if it doesn't exist
ENTRYPOINT="$PROJECT_ROOT/scripts/docker-entrypoint.sh"
if [ ! -f "$ENTRYPOINT" ]; then
    echo "Creating docker-entrypoint.sh..."
    cat > "$ENTRYPOINT" << 'ENTRYPOINT_CONTENT'
#!/bin/bash
# Docker entrypoint for Maximus BBS

set -e

MAXIMUS_HOME="/opt/maximus"
cd "$MAXIMUS_HOME"

# Check if this is first run (no user.bbs)
if [ ! -f "$MAXIMUS_HOME/etc/user.bbs" ]; then
    echo "First run detected - running initial configuration..."
    if [ -x "$MAXIMUS_HOME/bin/install.sh" ]; then
        # Run install in non-interactive mode
        "$MAXIMUS_HOME/bin/install.sh" --docker
    fi
fi

# If first argument is a flag, assume maxtel
if [ "${1#-}" != "$1" ]; then
    set -- bin/maxtel "$@"
fi

# If command is maxtel without path, add it
if [ "$1" = "maxtel" ]; then
    shift
    set -- bin/maxtel "$@"
fi

exec "$@"
ENTRYPOINT_CONTENT
    chmod +x "$ENTRYPOINT"
    echo "docker-entrypoint.sh created."
fi

# Create docker-compose.yml if it doesn't exist
COMPOSE="$PROJECT_ROOT/docker-compose.yml"
if [ ! -f "$COMPOSE" ]; then
    echo "Creating docker-compose.yml..."
    cat > "$COMPOSE" << 'COMPOSE_CONTENT'
# Maximus BBS Docker Compose Configuration
version: '3.8'

services:
  maximus:
    build: .
    image: maximus-bbs:latest
    container_name: maximus-bbs
    ports:
      - "2323:2323"
    volumes:
      - maximus-etc:/opt/maximus/etc
      - maximus-var:/opt/maximus/var
    restart: unless-stopped
    environment:
      - MAXIMUS_NODES=4
      - MAXIMUS_PORT=2323

volumes:
  maximus-etc:
  maximus-var:
COMPOSE_CONTENT
    echo "docker-compose.yml created."
fi

# Build the Docker image
echo ""
echo "Building Docker image..."
docker build -t "${IMAGE_NAME}:${VERSION}" -t "${IMAGE_NAME}:latest" .

echo ""
echo "=== Build Complete ==="
echo "Image: ${IMAGE_NAME}:${VERSION}"
echo ""
echo "To run:"
echo "  docker run -d -p 2323:2323 --name maximus ${IMAGE_NAME}:${VERSION}"
echo ""
echo "Or with docker-compose:"
echo "  docker-compose up -d"
echo ""

# Push if requested
if [ "$PUSH" = "true" ] || [ "$PUSH" = "push" ]; then
    echo "Pushing to registry..."
    docker push "${IMAGE_NAME}:${VERSION}"
    docker push "${IMAGE_NAME}:latest"
    echo "Push complete."
fi
