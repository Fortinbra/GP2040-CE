import { AppContext } from '../Contexts/AppContext';
import { useContext } from 'react';
import { Trans, useTranslation } from 'react-i18next';
import { FormCheck, Row, FormLabel } from 'react-bootstrap';
import { NavLink } from 'react-router-dom';
import * as yup from 'yup';

import Section from '../Components/Section';
import FormControl from '../Components/FormControl';

export const max17048Scheme = {
	MAX17048MonitorEnabled: yup.number().label('MAX17048 Monitor Enabled'),
	MAX17048MonitoringIntervalMs: yup.number().min(1000).max(60000).label('Monitoring Interval (ms)'),
	MAX17048AlertVoltageMin: yup.number().min(2.5).max(4.5).label('Alert Voltage Min (V)'),
	MAX17048AlertVoltageMax: yup.number().min(2.5).max(4.5).label('Alert Voltage Max (V)'),
	MAX17048EnableHibernation: yup.number().label('Enable Hibernation'),
	MAX17048HibernationThreshold: yup.number().min(0).max(100).label('Hibernation Threshold (%)'),
};

export const max17048State = {
	MAX17048MonitorEnabled: 0,
	MAX17048MonitoringIntervalMs: 5000,
	MAX17048AlertVoltageMin: 3.2,
	MAX17048AlertVoltageMax: 4.2,
	MAX17048EnableHibernation: 0,
	MAX17048HibernationThreshold: 5.0,
};

const MAX17048 = ({ values, errors, handleChange, handleCheckbox }) => {
	const { t } = useTranslation();
	const { getAvailablePeripherals } = useContext(AppContext);

	return (
		<Section title={
			<a
				href="https://gp2040-ce.info/add-ons/max17048-battery-monitor"
				target="_blank"
				rel="noreferrer"
				className="text-reset text-decoration-none"
			>
				{t('AddonsConfig:max17048-monitor-header-text')}
			</a>
		}
		>
			<div
				id="MAX17048MonitorOptions"
				hidden={!(values.MAX17048MonitorEnabled && getAvailablePeripherals('i2c'))}
			>
				<div className="alert alert-info" role="alert">
					The SDA and SCL pins and Speed are configured in <a href="../peripheral-mapping" className="alert-link">Peripheral Mapping</a>. 
					The MAX17048 device will be automatically detected at I2C address 0x36.
				</div>
				
				<Row className="mb-3">
					<FormControl
						type="number"
						label={t('AddonsConfig:max17048-monitoring-interval-label')}
						name="MAX17048MonitoringIntervalMs"
						className="form-control-sm"
						groupClassName="col-sm-3"
						value={values.MAX17048MonitoringIntervalMs}
						error={errors.MAX17048MonitoringIntervalMs}
						isInvalid={errors.MAX17048MonitoringIntervalMs}
						onChange={handleChange}
						min={1000}
						max={60000}
					/>
				</Row>

				<Row className="mb-3">
					<FormControl
						type="number"
						step="0.1"
						label={t('AddonsConfig:max17048-alert-voltage-min-label')}
						name="MAX17048AlertVoltageMin"
						className="form-control-sm"
						groupClassName="col-sm-3"
						value={values.MAX17048AlertVoltageMin}
						error={errors.MAX17048AlertVoltageMin}
						isInvalid={errors.MAX17048AlertVoltageMin}
						onChange={handleChange}
						min={2.5}
						max={4.5}
					/>
					<FormControl
						type="number"
						step="0.1"
						label={t('AddonsConfig:max17048-alert-voltage-max-label')}
						name="MAX17048AlertVoltageMax"
						className="form-control-sm"
						groupClassName="col-sm-3"
						value={values.MAX17048AlertVoltageMax}
						error={errors.MAX17048AlertVoltageMax}
						isInvalid={errors.MAX17048AlertVoltageMax}
						onChange={handleChange}
						min={2.5}
						max={4.5}
					/>
				</Row>

				<Row className="mb-3">
					<FormControl
						type="number"
						step="0.1"
						label={t('AddonsConfig:max17048-hibernation-threshold-label')}
						name="MAX17048HibernationThreshold"
						className="form-control-sm"
						groupClassName="col-sm-3"
						value={values.MAX17048HibernationThreshold}
						error={errors.MAX17048HibernationThreshold}
						isInvalid={errors.MAX17048HibernationThreshold}
						onChange={handleChange}
						min={0}
						max={100}
					/>
				</Row>
			</div>
			
			{getAvailablePeripherals('i2c') ? (
				<FormCheck
					label={t('Common:switch-enabled')}
					type="switch"
					id="MAX17048MonitorButton"
					reverse
					isInvalid={false}
					checked={
						Boolean(values.MAX17048MonitorEnabled) &&
						getAvailablePeripherals('i2c')
					}
					onChange={(e) => {
						handleCheckbox('MAX17048MonitorEnabled', values);
						handleChange(e);
					}}
				/>
			) : (
				<FormLabel>
					<Trans
						ns="PeripheralMapping"
						i18nKey="peripheral-toggle-unavailable"
						values={{ name: 'I2C' }}
					>
						<NavLink to="/peripheral-mapping">
							{t('PeripheralMapping:header-text')}
						</NavLink>
					</Trans>
				</FormLabel>
			)}
		</Section>
	);
};

export default MAX17048;
