import React from 'react';
import { Alert, Badge } from 'react-bootstrap';
import { getBoardConfig, getBoardDisplayName, getBoardFeatures, isBluetoothSupported } from '../Services/BoardConfig';

/**
 * BoardInfo Component
 * 
 * Displays information about the current board configuration
 * and conditionally shows features based on the target board.
 */
const BoardInfo = () => {
	const boardConfig = getBoardConfig();
	const boardDisplayName = getBoardDisplayName();
	const features = getBoardFeatures();

	return (
		<div className="board-info">
			<Alert variant="info">
				<Alert.Heading>Board Configuration</Alert.Heading>
				<p>
					<strong>Board:</strong> {boardDisplayName} 
					<Badge bg="secondary" className="ms-2">
						{boardConfig.picoBoardType}
					</Badge>
				</p>
				<p>
					<strong>Configuration:</strong> {boardConfig.gp2040BoardConfig}
				</p>
				
				<div className="features mt-3">
					<h6>Available Features:</h6>
					<div className="d-flex gap-2 flex-wrap">
						{features.bluetooth && (
							<Badge bg="success">Bluetooth</Badge>
						)}
						{features.wifi && (
							<Badge bg="primary">WiFi</Badge>
						)}
						{features.enhancedPerformance && (
							<Badge bg="warning">Enhanced Performance</Badge>
						)}
						{features.wirelessGamepad && (
							<Badge bg="info">Wireless Gamepad</Badge>
						)}
					</div>
				</div>

				{isBluetoothSupported() && (
					<Alert variant="success" className="mt-3 mb-0">
						<strong>Bluetooth Enabled:</strong> This board supports wireless connectivity
						via Bluetooth HID. You can pair this gamepad with compatible devices.
					</Alert>
				)}
			</Alert>

			{/* Conditional rendering based on board type */}
			{boardConfig.isPicoW && (
				<Alert variant="warning">
					<strong>Pico W Notice:</strong> Some GPIO pins are reserved for the wireless chip.
					Please refer to the documentation for pin restrictions.
				</Alert>
			)}

			{boardConfig.isPico2W && (
				<Alert variant="info">
					<strong>Pico 2 W Features:</strong> This board combines the enhanced RP2350 
					processor with wireless capabilities for the best performance and connectivity.
				</Alert>
			)}
		</div>
	);
};

export default BoardInfo;
