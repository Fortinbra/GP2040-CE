import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import path from "path";

// import dev host for wsl2
const host = process.env.VITE_DEV_HOST || 'localhost';

// Board configuration from CMake build
const picoBoardType = process.env.VITE_PICO_BOARD || 'pico';
const gp2040BoardConfig = process.env.VITE_GP2040_BOARDCONFIG || 'Pico';

console.log(`Building web component for PICO_BOARD: ${picoBoardType}, GP2040_BOARDCONFIG: ${gp2040BoardConfig}`);

// https://vitejs.dev/config/
export default defineConfig({
	build: {
		outDir: path.join(__dirname, "build"),
		sourcemap: false,
	},
	server: {
		host: host,
		open: true,
		port: 3000,
	},
	plugins: [react()],
	resolve: {
		alias: {
			"~bootstrap": path.resolve(__dirname, "node_modules/bootstrap"),
			lodash: 'lodash-es',
			"@proto": path.resolve(__dirname, "src_gen"),
		},
	},
	define: {
		// Make board configuration available at build time
		__PICO_BOARD__: JSON.stringify(picoBoardType),
		__GP2040_BOARDCONFIG__: JSON.stringify(gp2040BoardConfig),
		__BUILD_BLUETOOTH_ENABLED__: JSON.stringify(
			picoBoardType === 'pico_w' || picoBoardType === 'pico2_w'
		),
	},
});
