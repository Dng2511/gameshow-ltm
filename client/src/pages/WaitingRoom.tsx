import { useState, useEffect } from "react";
import { useNavigate, useParams } from "react-router-dom";
import toast from "react-hot-toast";
import axios from 'axios';

const BASE_URL = import.meta.env.VITE_SERVER_URL

export const WaitingRoom = () => {
  const { id } = useParams<{ id: string }>();
  const [loading, setLoading] = useState(false);
  const [roomInfo, setRoomInfo] = useState<any>(null);
  const navigate = useNavigate();
  const [eventSource, setEventSource] = useState<EventSource | null>(null);

  const username = localStorage.getItem("username")

  const fetchData = async () => {
    setLoading(true);
    try {
      const res = await axios.post(`${BASE_URL}/room/get_info`, {
        room_name: id,
      },
      {
        headers: {
          'Content-Type': 'application/json',
        },
      });
      setRoomInfo(res.data);
      console.log(res.data)
    } catch (err: any) {
      console.log(err);
      if (err.message) {
        toast.error(err.message);
      }
    } finally {
      setLoading(false);
    }
  };

  // Start Server-Sent Events (SSE)
  const startSSE = () => {
    const es = new EventSource(`${BASE_URL}/subscribe`);
    setEventSource(es);
  
    es.onmessage = (event) => {
      console.log("SSE event data:", event.data);
      const data = JSON.parse(event.data);
      if (data.action === "join" && data.username !== username && data.room_name === id) {
        setRoomInfo((prevRoomInfo: any) => ({
          ...prevRoomInfo,
          users: [...prevRoomInfo.users, { username: data.username }]
        }));
      } else if (data.action === "leave" && data.username !== username && data.room_name === id) {
        setRoomInfo((prevRoomInfo: any) => ({
          ...prevRoomInfo,
          users: prevRoomInfo.users.filter((user: any) => user.username !== data.username)
        }));
      } else if (data.action === "disband" && data.room_name === id) {
        es.close();
        navigate("/");
      } else if (data.action === "start" && data.room_name === id) {
        es.close();
        navigate(`/game/${id}`);
      }
    };
  
    es.onerror = () => {
      console.error('SSE connection error. Reconnecting...');
      es.close();
      navigate("/error")
    };
  };

  useEffect(() => {
    if (id) {
      startSSE();
      fetchData();
    }

    return () => {
      if (eventSource) {
        eventSource.close();
        console.log("EventSource closed in WaitingRoom");
      }
    };
  }, [id]);

  const handleLeaveRoom = async () => {
    if (loading) return;

    setLoading(true);
    try {
      const res = await axios.post(`${BASE_URL}/room/leave`, {room_name: id,username} ,{
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true
      });
      console.log(res.data);
      if (res.data.status === "success"){
        toast.success(res.data.message);
        if (eventSource) {
          eventSource.close();
          console.log("EventSource closed in WaitingRoom");
        }
        navigate(`/`);
      }
    } catch (err: any) {
      console.log(err)
      if (err.response.data){
        toast.error(err.response.data);
      }
    } finally {
      setLoading(false);
    }
  };

  const handleStartGame = async () => {
    if (loading) return;

    setLoading(true);
    try {
      console.log({
        room_name: id,
        num_players: roomInfo.users.length
      })
      const res = await axios.post(`${BASE_URL}/game/init`, {
        room_name: id,
        username,
        num_players: roomInfo.users.length
      } ,{
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true
      });
      console.log(res.data);
      if (res.data.status === "Game initialized"){
        toast.success("Game started");
        if (eventSource) {
          eventSource.close();
          console.log("EventSource closed in WaitingRoom");
        }
        navigate(`/game/${id}`);
      }
    } catch (err: any) {
      console.log(err)
      if (err.response.data.message){
        toast.error(err.data.message);
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="h-dvh relative">
      <div className="size-full brightness-[.25] transition duration-300">
        <img className="object-cover size-full" src="/waiting.jpg" alt="waiting" />
      </div>
      <div className="fixed top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 flex flex-col gap-8 items-center">
        {loading ? (
          <div className="text-5xl text-white/90 font-bold text-center">
            Loading...
          </div>
        ) : (
          roomInfo && (
            <>
              <div className="text-5xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
                <span>Waiting room: 
                  <span className="text-slate-700/80"> {roomInfo.room_name}</span>
                </span>
              </div>
              <div className="flex gap-24 ">
                <div className="text-3xl text-white/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>Host: {roomInfo.host}</span>
                </div>
                <div className="text-3xl text-white/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>Topic: {roomInfo.topic}</span>
                </div>
              </div>
              <div className="text-3xl text-white/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>Players: {roomInfo.users.length}/{roomInfo.capacity}</span>
              </div>
              <div className="flex flex-wrap gap-8 justify-around">
              {roomInfo.users.map((user: any)=>(
                <div key={user.username} className="text-3xl bg-white/90 text-black/80 rounded-xl px-4 py-2 font-bold text-center">
                  <span>{user.username} {user.username===username && "(You)"}</span>
                </div>
              ))}
              </div>

            </>
          )
        )}
      </div>
      <div className="fixed top-8 left-6 flex gap-8 text-rose-400 px-4 py-1">
        {roomInfo &&username === roomInfo.host ? <>
          <button onClick={handleLeaveRoom} className='text-lg font-bold px-6 py-6 rounded-full border-2 border-rose-400 hover:bg-rose-400 hover:text-white transition duration-300'>
            Disband room
          </button>
        </> : <>
          <button onClick={handleLeaveRoom} className='text-lg font-bold px-6 py-6 rounded-full border-2 border-rose-400 hover:bg-rose-400 hover:text-white transition duration-300'>
            Leave room
          </button>
        </>}
      </div>
      <div className="fixed top-8 right-6 flex gap-8 text-lime-400 px-4 py-1">
        {roomInfo && username === roomInfo.host &&
          <button onClick={handleStartGame} className='text-lg font-bold px-6 py-6 rounded-full border-2 border-lime-400 hover:bg-lime-400 hover:text-slate-600 transition duration-300'>
            Start game
          </button>
         }
      </div>
    </div>
  )
}