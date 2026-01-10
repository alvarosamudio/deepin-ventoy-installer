# Deepin Ventoy Installer

A modern, easy-to-use Ventoy installer front-end built with DTK (Deepin ToolKit). This application simplifies the process of creating bootable USB drives with Ventoy on Deepin and other Linux distributions.

## Features

- **Intuitive UI**: Built with DTK for a native look and feel on Deepin.
- **Drag & Drop**: Simply drop your Ventoy `tar.gz` archive to start.
- **Automatic Download**: Fetch the latest version of Ventoy directly from GitHub with one click.
- **Smart Detection**: Automatically identifies connected USB drives and their current Ventoy versions.
- **Multilingual Support**: Available in English, Chinese, and Spanish.
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

## Build

### Prerequisites

- qt5-default
- libdtkwidget-dev
- qttools5-dev-tools

### Compile

```bash
# Clone the repository
git clone https://github.com/alvarosamudio/deepin-ventoy-installer.git
cd deepin-ventoy-installer

qmake
make
```

## Running

```bash
sudo ./DeepinVentoyInstaller
```

## Translations

Translations are located in the `translations/` directory. Contributions are welcome!

- English (Default)
- Chinese (`dvtoyinst_zh.ts`)
- Spanish (`dvtoyinst_es.ts`)

## License

This project is licensed under the GPL-3.0 License.
