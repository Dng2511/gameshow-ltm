import './App.css'
import { BrowserRouter, Route, Routes } from "react-router-dom";
import routes from './routes/routes';
import { ToasterProvider } from "./providers/ToasterProvider";


function App() {

  return (
    <>
      <ToasterProvider>
        <BrowserRouter>
            <Routes>
              <Route path="/">
                {routes.map((route, index) =>
                    <Route
                    path={route.path}
                    key={index}
                    element={route.element}
                    />
                  )}
              </Route>
            </Routes>
        </BrowserRouter>
      </ToasterProvider>
    </>
  )
}

export default App
