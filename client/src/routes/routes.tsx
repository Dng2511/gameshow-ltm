import { Error } from "@/pages/Error";
import { Home } from "@/pages/Home";
import { IngameRoom } from "@/pages/IngameRoom";
import { Leaderboard } from "@/pages/Leaderboard";
import { Result } from "@/pages/Result";
import { WaitingRoom } from "@/pages/WaitingRoom";

const routes = [
    {
      path: "/",
      element: <Home />,
    },
    {
      path: "/room/:id",
      element: <WaitingRoom />,
    },
    {
      path: "/game/:id",
      element: <IngameRoom />,
    },
    {
      path: "/leaderboard/:id",
      element: <Leaderboard />,
    },
    {
      path: "/result/:id",
      element: <Result />,
    },
    {
      path: "/error",
      element: <Error />,
    },
  ];
  
  export default routes;