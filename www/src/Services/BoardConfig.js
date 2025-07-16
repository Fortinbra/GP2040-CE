/**
 * Board Configuration Utilities
 * 
 * Provides access to board-specific configuration and build-time constants
 * injected from the CMake build process.
 */

export interface BoardConfig {
	picoBoardType: string;
	gp2040BoardConfig: string;
	isBluetoothEnabled: boolean;
	isPicoW: boolean;
	isPico2W: boolean;
	isWirelessBoard: boolean;
}

/**
 * Get the current board configuration
 */
export function getBoardConfig(): BoardConfig {
	// In development, use environment variables or defaults
	// In production, use build-time constants
	const picoBoardType = typeof __PICO_BOARD__ !== 'undefined' 
		? __PICO_BOARD__ 
		: import.meta.env.VITE_PICO_BOARD || 'pico';
	
	const gp2040BoardConfig = typeof __GP2040_BOARDCONFIG__ !== 'undefined'
		? __GP2040_BOARDCONFIG__
		: import.meta.env.VITE_GP2040_BOARDCONFIG || 'Pico';
	
	const isBluetoothEnabled = typeof __BUILD_BLUETOOTH_ENABLED__ !== 'undefined'
		? __BUILD_BLUETOOTH_ENABLED__
		: (picoBoardType === 'pico_w' || picoBoardType === 'pico2_w');

	const isPicoW = picoBoardType === 'pico_w';
	const isPico2W = picoBoardType === 'pico2_w';
	const isWirelessBoard = isPicoW || isPico2W;

	return {
		picoBoardType,
		gp2040BoardConfig,
		isBluetoothEnabled,
		isPicoW,
		isPico2W,
		isWirelessBoard,
	};
}

/**
 * Check if the current board supports Bluetooth
 */
export function isBluetoothSupported(): boolean {
	return getBoardConfig().isBluetoothEnabled;
}

/**
 * Check if the current board is a wireless variant (Pico W or Pico 2 W)
 */
export function isWirelessBoard(): boolean {
	return getBoardConfig().isWirelessBoard;
}

/**
 * Get board-specific display name
 */
export function getBoardDisplayName(): string {
	const config = getBoardConfig();
	
	switch (config.picoBoardType) {
		case 'pico_w':
			return 'Raspberry Pi Pico W';
		case 'pico2_w':
			return 'Raspberry Pi Pico 2 W';
		case 'pico2':
			return 'Raspberry Pi Pico 2';
		case 'pico':
		default:
			return 'Raspberry Pi Pico';
	}
}

/**
 * Get available features based on board configuration
 */
export function getBoardFeatures() {
	const config = getBoardConfig();
	
	return {
		bluetooth: config.isBluetoothEnabled,
		wifi: config.isWirelessBoard,
		enhancedPerformance: config.picoBoardType.includes('pico2'),
		wirelessGamepad: config.isBluetoothEnabled,
	};
}
