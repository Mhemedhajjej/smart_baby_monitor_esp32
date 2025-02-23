# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.0.1] - 2025-02-23
### Added
- Add support of room temperature/humidity monitoring as part of baby alert system.
- Add support of baby sound detector to the alert notification system.
- Add baby status button as option for user to notify ESP that user got already the alert and is about taking care of the baby
- Support websocket for data exchange between ESP32 camera web server and clients.
- Add support of send alert notification to user via Email SMTP method.
- Add support of send alert notification to user via WhatsApp method.
- Add alert notification via logging

### Changed
- Cleanup code and restrict camera web server to options meaningful for user (reduce firmware size).
- Change web site design

### Fixed
- No specific fixes mentioned in the commits.

## [1.0.0] - 2025-01-26
### Added
- Initial release of the Smart Baby Monitor ESP32.
- Import the ESP32 CameraWebServer Arduino example as base for the application.
- Code refactoring to re-direct the base code to the baby monitor application.
- Add support Camera Web Server rebuild from ESP32 CameraWebServer Arduino example.
- Add support of file conversion HTML->C for web server to allow modification of the web server via HTML.