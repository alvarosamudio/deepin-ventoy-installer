# Deepin Ventoy Installer

A modern, easy-to-use Ventoy installer front-end built with DTK (Deepin ToolKit). This application simplifies the process of creating bootable USB drives with Ventoy on Deepin and other Linux distributions.

## Features

- **Intuitive UI**: Built with DTK for a native look and feel on Deepin.
- **Drag & Drop**: Simply drop your Ventoy `tar.gz` archive to start.
- **Automatic Download**: Fetch the latest version of Ventoy directly from GitHub with one click.
- **Smart Detection**: Automatically identifies connected USB drives and their current Ventoy versions.
- **Multilingual Support**: Available in English, Chinese, Spanish, Portuguese, Italian, and French.
- **Safe Operations**: Includes root elevation checks and multiple confirmation prompts to prevent accidental data loss.
- **Advanced Options**: Support for GPT partition tables and Secure Boot.

## Prerequisites

- Deepin Operating System (or any distribution with DTK installed)
- Root privileges (for writing to USB drives)
- Internet connection (for the "Download Latest" feature)

## Supported Languages

- English
- Chinese (Simplified)
- Spanish
- Portuguese
- Italian
- French

## Installation

### From DEB Package (Recommended)

Download the latest DEB package from the [Releases page](https://github.com/alvarosamudio/deepin-ventoy-installer/releases) and install with:

```bash
sudo dpkg -i deepin-ventoy-installer_*.deb
sudo apt-get install -f  # Install any missing dependencies
```

### Building from Source

#### Prerequisites

- qt5-default
- libdtkwidget-dev
- qttools5-dev-tools

#### Compile

```bash
# Clone the repository
git clone https://github.com/alvarosamudio/deepin-ventoy-installer.git
cd deepin-ventoy-installer

qmake
make
```

## Running

```bash
sudo ./dvtoyinst
```

Or from the application menu after installing the DEB package.

## Translations

Translations are managed using Qt Linguist and stored in the `translations/` directory. Contributions are welcome!

| Language | File |
|----------|------|
| English | (Default) |
| Chinese (Simplified) | `dvtoyinst_zh.ts` |
| Spanish | `dvtoyinst_es.ts` |
| Portuguese | `dvtoyinst_pt.ts` |
| Italian | `dvtoyinst_it.ts` |
| French | `dvtoyinst_fr.ts` |

To update translations:
1. Edit the `.ts` files using Qt Linguist
2. Compile translations: `lrelease translations/*.ts`
3. Submit a pull request

## GitHub Actions

This project uses GitHub Actions for CI/CD. The workflow:
1. Builds the application on Ubuntu
2. Compiles translations
3. Creates a DEB package with maximum compression (xz -9)
4. Uploads the package as a build artifact

## License

This project is licensed under the GPL-3.0 License.
