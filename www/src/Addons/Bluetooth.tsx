import { useTranslation } from 'react-i18next';
import { FormCheck, Row, Col, Button, Form } from 'react-bootstrap';
import * as yup from 'yup';

import Section from '../Components/Section';
import FormControl from '../Components/FormControl';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';

export const bluetoothScheme = {
    BluetoothAddonEnabled: yup
        .number()
        .label('Bluetooth Addon Enabled'),
    bluetoothPairingMode: yup
        .number()
        .label('Bluetooth Pairing Mode')
        .validateRangeWhenValue('BluetoothAddonEnabled', 0, 1),
    bluetoothBondedDeviceName: yup
        .string()
        .label('Bluetooth Bonded Device Name'),
    bluetoothBondedDeviceAddr: yup
        .string()
        .label('Bluetooth Bonded Device Address'),
};

export const bluetoothState = {
    BluetoothAddonEnabled: 0,
    bluetoothPairingMode: 0,
    bluetoothBondedDeviceName: '',
    bluetoothBondedDeviceAddr: '',
};

const Bluetooth = ({
    values,
    errors,
    handleChange,
    handleCheckbox,
    setFieldValue,
}: AddonPropTypes) => {
    const { t } = useTranslation();

    const handleClearPairing = () => {
        setFieldValue('bluetoothBondedDeviceAddr', '');
        setFieldValue('bluetoothBondedDeviceName', '');
    };

    return (
        <Section
            title={
                <a
                    href="https://gp2040-ce.info/add-ons/bluetooth"
                    target="_blank"
                    className="text-reset text-decoration-none"
                >
                    {t('AddonsConfig:bluetooth-header-text')}
                </a>
            }
        >
            <div id="BluetoothOptions" hidden={!values.BluetoothAddonEnabled}>
                <Row className="mb-3">
                    <Col sm={12}>
                        <div className="alert alert-info" role="alert">
                            {t('AddonsConfig:bluetooth-info-text')}
                        </div>
                    </Col>
                </Row>
                <Row className="mb-3">
                    <FormCheck
                        label={t('AddonsConfig:bluetooth-pairing-mode-label')}
                        type="switch"
                        id="bluetoothPairingMode"
                        className="col-sm-6"
                        isInvalid={false}
                        checked={Boolean(values.bluetoothPairingMode)}
                        onChange={(e) => {
                            handleCheckbox('bluetoothPairingMode');
                            handleChange(e);
                        }}
                    />
                </Row>
                <Row className="mb-3">
                    <Col sm={6}>
                        <Form.Label>
                            {t('AddonsConfig:bluetooth-paired-device-label')}
                        </Form.Label>
                        <FormControl
                            type="text"
                            name="bluetoothBondedDeviceName"
                            className="form-control-sm"
                            value={values.bluetoothBondedDeviceName || t('AddonsConfig:bluetooth-no-device-paired')}
                            readOnly
                            disabled
                        />
                    </Col>
                    <Col sm={6} className="d-flex align-items-end">
                        <Button
                            variant="danger"
                            size="sm"
                            onClick={handleClearPairing}
                            disabled={!values.bluetoothBondedDeviceName}
                        >
                            {t('AddonsConfig:bluetooth-clear-pairing-button')}
                        </Button>
                    </Col>
                </Row>
            </div>
            <FormCheck
                label={t('Common:switch-enabled')}
                type="switch"
                id="BluetoothAddonButton"
                reverse
                isInvalid={false}
                checked={Boolean(values.BluetoothAddonEnabled)}
                onChange={(e) => {
                    handleCheckbox('BluetoothAddonEnabled');
                    handleChange(e);
                }}
            />
        </Section>
    );
};

export default Bluetooth;
