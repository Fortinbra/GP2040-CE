import { lazy, Suspense } from 'react';
import { Spinner } from 'react-bootstrap';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import { AppContextProvider } from './Contexts/AppContext';

import Navigation from './Components/Navigation';

import './App.scss';

const HomePage = lazy(() => import('./Pages/HomePage'));
const PinMappingPage = lazy(() => import('./Pages/PinMapping'));
const PeripheralMappingPage = lazy(
	() => import('./Pages/PeripheralMappingPage'),
);
const ResetSettingsPage = lazy(() => import('./Pages/ResetSettingsPage'));
const SettingsPage = lazy(() => import('./Pages/SettingsPage'));
const DisplayConfigPage = lazy(() => import('./Pages/DisplayConfig'));
const AddonsConfigPage = lazy(() => import('./Pages/AddonsConfigPage'));
const BackupPage = lazy(() => import('./Pages/BackupPage'));
const PlaygroundPage = lazy(() => import('./Pages/PlaygroundPage'));
const InputMacroAddonPage = lazy(() => import('./Pages/InputMacroAddonPage'));
const LedConfigPage = lazy(() => import('./Pages/LedConfigPage'));
const BootModeMappingPage = lazy(() => import('./Pages/BootModeMapping'));

const App = () => {
	const { t } = useTranslation();

	return (
		<AppContextProvider>
			<Router>
				<Navigation />
				<div className="body-content container-lg">
					<Suspense
						fallback={
							<div
								className="d-flex justify-content-center mt-4"
								role="status"
							>
								<Spinner animation="border" aria-hidden="true" />
								<span className="visually-hidden">
									{t('Common:loading-text')}
								</span>
							</div>
						}
					>
						<Routes>
							<Route path="/" element={<HomePage />} />
							<Route path="/settings" element={<SettingsPage />} />
							<Route path="/pin-mapping" element={<PinMappingPage />} />
							<Route
								path="/boot-mode-mapping"
								element={<BootModeMappingPage />}
							/>
							<Route
								path="/peripheral-mapping"
								element={<PeripheralMappingPage />}
							/>
							<Route path="/reset-settings" element={<ResetSettingsPage />} />
							<Route path="/led-config" element={<LedConfigPage />} />
							<Route path="/display-config" element={<DisplayConfigPage />} />
							<Route path="/add-ons" element={<AddonsConfigPage />} />
							<Route path="/backup" element={<BackupPage />} />
							<Route path="/playground" element={<PlaygroundPage />} />
							<Route path="/macro" element={<InputMacroAddonPage />} />
						</Routes>
					</Suspense>
				</div>
			</Router>
		</AppContextProvider>
	);
};

export default App;
