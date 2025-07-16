/// <reference types="vite/client" />

interface ImportMetaEnv {
	readonly VITE_GP2040_BOARD: string;
	readonly VITE_GP2040_CONTROLLER: string;
	readonly VITE_DEV_BASE_URL: string;
	readonly VITE_DEV_HOST: string;
	readonly VITE_PICO_BOARD: string;
	readonly VITE_GP2040_BOARDCONFIG: string;
}

interface ImportMeta {
	readonly env: ImportMetaEnv;
}

// Build-time constants injected by Vite
declare const __PICO_BOARD__: string;
declare const __GP2040_BOARDCONFIG__: string;
declare const __BUILD_BLUETOOTH_ENABLED__: boolean;
