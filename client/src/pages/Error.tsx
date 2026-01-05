import { useNavigate } from "react-router-dom";

export const Error = () => {

  const navigate = useNavigate();

  return (
    <div className="h-dvh relative flex items-center justify-center">
      <div className="size-full brightness-[.2] transition duration-300">
        <img className="object-cover size-full" src="/maintenance.jpg" alt="maintenance" />
      </div>
      <div className="absolute top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-32 text-center">
        <div className="text-6xl font-bold text-white">
          <span>Oh, no...</span>
        </div>
        <div className="text-3xl tracking-wide font-bold text-white">
          <span>Server has been shut down unexpectedly  </span>
        </div>
      </div>
      <div className="fixed bottom-4 left-28 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-4">
        <div onClick={() => navigate(`/`)} className="text-2xl text-white/90 rounded-xl px-4 py-1 font-bold flex items-center justify-center cursor-pointer hover:underline">
          <span>Back to home</span>
        </div>
      </div>
    </div>
  )
}
