# sbbdep (Slack Build Binary Dependencies)

sbbdep is a tool for Slackware and Slackware based distributions that traces
binary runtime dependencies of dynamic linked files.

sbbdep has been around since 2010. After a quiet period from 2018 it is back
in maintenance mode as of 2026. Since the tool is feature complete, this
revival is released as version 1.0.0.

Tested on Slackware current as of March 2026.

## Package notes

Build:

```sh
cmake --workflow --preset release
```

Install into a staging directory (e.g. for Slackware packaging):

```sh
PKG=/path/to/staging

DESTDIR="$PKG" cmake --install build/ninja \
  --config Release \
  --prefix /usr \
  --component sbbdep
```

### Create a package from source

Run

```sh
slackbuild/create-pkg.sh
```

then install or upgrade:

```sh
sudo upgradepkg --install-new tmp/sbbdep-*.tgz
```

### Preview documentation locally

```sh
asciidoctor -a relfilesuffix=.html docs/*.adoc -D _site
xdg-open _site/index.html
```

### Shell completion

Shell completion requires the `bash-completion` package. On Slackware it is
tagged optional and may not be installed even on a full install:

```sh
slackpkg install bash-completion
```

## Usage examples

### Who uses a library?

Find all packages and binaries that depend on `libssl.so.3`:

```text
sbbdep --whoneeds --xdl /usr/lib64/libssl.so.3

/lib64/libssl.so.3 (libssl.so.3) is used from:
  aaa_libraries-15.1-x86_64-49
    /usr/lib64/libcups.so.2
    /usr/lib64/libcurl.so.4.8.0
    /usr/lib64/libldap.so.2.0.200
    /usr/lib64/libssh2.so.1.0.1
  alpine-2.29.9-x86_64-1
    /usr/bin/alpine
    /usr/bin/pico
  bind-9.20.20-x86_64-1
    /usr/bin/dnssec-cds
    /usr/bin/dnssec-keyfromlabel
```

### What does a library need?

Find which packages provide the libraries that `libssl.so.3` itself depends on:

```text
sbbdep --xdl --short /usr/lib64/libssl.so.3

check /lib64/libssl.so.3, 64bit library (libssl.so.3)
 .. from package openssl-solibs-3.5.5-x86_64-1
 .. from package openssl-3.5.5-x86_64-1

file /lib64/libssl.so.3 needs:
  libc.so.6 found in:
    /lib64/libc-2.42.so (aaa_glibc-solibs | glibc)
  libcrypto.so.3 found in:
    /lib64/libcrypto.so.3 (openssl | openssl-solibs)
  libz.so.1 found in:
    /lib64/libz.so.1.3.2 (aaa_libraries | zlib)
```

### Package dependencies

Show all runtime dependencies of a package:

```text
sbbdep /var/adm/packages/curl-8.19.0-x86_64-1

aaa_glibc-solibs = 2.42-x86_64-1 | glibc = 2.42-x86_64-1
aaa_libraries = 15.1-x86_64-49 | c-ares = 1.34.6-x86_64-1
aaa_libraries = 15.1-x86_64-49 | krb5 = 1.22.2-x86_64-1
aaa_libraries = 15.1-x86_64-49 | libidn2 = 2.3.8-x86_64-1
aaa_libraries = 15.1-x86_64-49 | libpsl = 0.21.5-x86_64-1
aaa_libraries = 15.1-x86_64-49 | libssh2 = 1.11.1-x86_64-1
aaa_libraries = 15.1-x86_64-49 | zlib = 1.3.2-x86_64-1
brotli = 1.2.0-x86_64-1
nghttp2 = 1.68.0-x86_64-1
openssl = 3.5.5-x86_64-1 | openssl-solibs = 3.5.5-x86_64-1
```

The output format is a package list as slapt-get expects it for dependencies.
(The `--short` option generates a list without version numbers.)

### Who depends on a package?

Show all packages that require curl:

```text
sbbdep --whoneeds /var/adm/packages/curl-8.19.0-x86_64-1

NetworkManager-1.56.0-x86_64-1
cmake-4.2.3-x86_64-1
curl-8.19.0-x86_64-1
elfutils-0.194-x86_64-1
exiv2-0.28.8-x86_64-1
```

### Non-binary file lookup

If the argument is not a binary, sbbdep searches the package database:

```text
sbbdep /etc/ssl/openssl.cnf

not a file with binary dependencies: /etc/ssl/openssl.cnf
 try to find information in package list:
 filename found in /var/adm/packages/stunnel-5.77-x86_64-1: openssl.cnf
 filename found in /var/adm/packages/openvpn-2.7.0-x86_64-1: openssl.cnf
absolute match in /var/adm/packages/openssl-solibs-3.5.5-x86_64-1: /etc/ssl/openssl.cnf
absolute match in /var/adm/packages/openssl-3.5.5-x86_64-1: /etc/ssl/openssl.cnf
```

For more information and a detailed overview, visit the [wiki](https://a4z.github.io/sbbdep/).
