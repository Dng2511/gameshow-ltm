import { zodResolver } from "@hookform/resolvers/zod";
import { useForm } from "react-hook-form";
import { z } from "zod";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "../components/ui/form";
import {
    Select,
    SelectContent,
    SelectItem,
    SelectTrigger,
    SelectValue,
  } from "@/components/ui/select"
import { Input } from "../components/ui/input"
import { useState } from "react";
import toast from "react-hot-toast";
import { cn } from "@/lib/utils";
import axios from 'axios';
import { useNavigate } from 'react-router-dom';

const BASE_URL = import.meta.env.VITE_SERVER_URL

type Props = {
  isOpen: boolean;
  setIsOpen: React.Dispatch<React.SetStateAction<boolean>>;
};

const CreateRoom: React.FC<Props> = ({ isOpen, setIsOpen }) => {
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();

  const username = localStorage.getItem("username")

  const formSchema = z.object({
    room_name: z.string().min(1, {
      message: "Room name is required",
    }),
    capacity: z.enum(["5", "10", "20"], {
      errorMap: () => ({ message: "Capacity must be one of 5, 10, or 20" }),
    }),
    topic: z.enum(["anime", "country", "youtuber"], {
      errorMap: () => ({ message: "Topic must be one of anime, country, or youtuber" }),
    }),
    num_questions: z.enum(["5","10"], { errorMap: () => ({ message: "Num questions must be 5,10" })}),
    username: z.string().nullable()
  });

  const form = useForm<z.infer<typeof formSchema>>({
    resolver: zodResolver(formSchema),
    defaultValues: {
      room_name: "",
      capacity: "5",
      topic: "anime",
      num_questions: "5",
      username: username
    },
  });

  const onSubmit = async (values: z.infer<typeof formSchema>) => {
    if (loading) return;

    values.username = username;

    console.log(values);
    setLoading(true);
    try {
      const res = await axios.post(`${BASE_URL}/room/create`, values, {
        headers: {
          'Content-Type': 'application/json',
        },
        withCredentials: true
      });
      console.log(res.data);
      if (res.data.message){
        if (res.data.status === "success"){
          toast.success(res.data.message);
          navigate(`/room/${values.room_name}`);
        } else {
          toast.error(res.data.message);
        }
      }
      setIsOpen(false);
      form.reset();
    } catch (err: any) {
      console.log(err)
      if (err.response.data){
        toast.error(err.response.data);
      }
    } finally {
      setLoading(false);
    }
  };

  return (
    <>
      {isOpen && (
        <div
          onClick={() => setIsOpen(false)}
          className="flex bg-black/30 backdrop-blur-[2px] overflow-y-auto overflow-x-hidden fixed top-0 right-0 left-0 z-50 justify-center items-center w-full md:inset-0 h-full max-h-full"
        >
          <div
            onClick={(e) => e.stopPropagation()}
            className="relative p-4 w-full max-w-xl max-h-full"
          >
            {/* Modal content */}
            <div className="relative rounded-lg shadow bg-gray-800/90 border-[1px] border-white">
              {/* Modal header */}
              <div className="flex items-center justify-between p-4 md:p-6 border-b rounded-t border-gray-600">
                <h3 className="text-3xl font-semibold text-white">
                  Create new room
                </h3>
                <button
                  onClick={() => setIsOpen(false)}
                  type="button"
                  className="end-2.5 text-gray-400 bg-transparent rounded-lg text-lg w-8 h-8 ms-auto inline-flex justify-center items-center hover:bg-gray-600 hover:text-white"
                  data-modal-hide="authentication-modal"
                >
                  <svg
                    className="w-3 h-3"
                    aria-hidden="true"
                    xmlns="http://www.w3.org/2000/svg"
                    fill="none"
                    viewBox="0 0 14 14"
                  >
                    <path
                      stroke="currentColor"
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      strokeWidth="2"
                      d="m1 1 6 6m0 0 6 6M7 7l6-6M7 7l-6 6"
                    />
                  </svg>
                  <span className="sr-only">Close modal</span>
                </button>
              </div>
              {/* Modal body */}
              <div className="p-4 md:px-6 md:py-4 mb-2">
                <Form {...form}>
                  <form
                    className="space-y-6"
                    onSubmit={form.handleSubmit(onSubmit)}
                  >
                    <FormField
                      control={form.control}
                      name="room_name"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel className="block mb-2 text-lg font-medium text-white text-start">
                            Room Name
                          </FormLabel>
                          <FormControl>
                            <Input
                              {...field}
                              className="text-lg font-semibold rounded-lg block w-full p-2.5 bg-white border-gray-500 focus:border-gray-600 placeholder-gray-400 text-black ring-offset-gray-400"
                              placeholder="Room name"
                            />
                          </FormControl>
                          <FormMessage />
                        </FormItem>
                      )}
                    />
                    <FormField
                      control={form.control}
                      name="capacity"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel className="block mb-2 text-lg font-medium text-white text-start">
                            Capacity
                          </FormLabel>
                          <Select onValueChange={field.onChange} defaultValue={field.value}>
                            <FormControl>
                              <SelectTrigger>
                                <SelectValue className="font-semibold" placeholder="Select capacity" />
                              </SelectTrigger>
                            </FormControl>
                            <SelectContent className="font-semibold">
                              <SelectItem value="5">5</SelectItem>
                              <SelectItem value="10">10</SelectItem>
                            </SelectContent>
                          </Select>
                          <FormMessage />
                        </FormItem>
                      )}
                    />
                    <FormField
                      control={form.control}
                      name="topic"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel className="block mb-2 text-lg font-medium text-white text-start">
                            Topic
                          </FormLabel>
                          <Select onValueChange={field.onChange} defaultValue={field.value}>
                            <FormControl>
                              <SelectTrigger>
                                <SelectValue placeholder="Select topic" />
                              </SelectTrigger>
                            </FormControl>
                            <SelectContent className="font-semibold">
                              <SelectItem value="anime">Anime</SelectItem>
                              <SelectItem value="country">Country</SelectItem>
                              <SelectItem value="youtuber">Youtuber</SelectItem>
                            </SelectContent>
                          </Select>
                          <FormMessage />
                        </FormItem>
                      )}
                    />

                    <button
                      type="submit"
                      className={cn(
                        "w-full text-white focus:ring-4 focus:outline-none focus:ring-blue-300 font-medium rounded-lg text-lg px-5 py-2.5 text-center bg-sky-500 hover:bg-sky-600",
                        {
                          "bg-gray-600 hover:bg-gray-600 select-none cursor-progress":
                            loading,
                        }
                      )}
                      disabled={loading}
                    >
                      Create Room
                    </button>
                  </form>
                </Form>
              </div>
            </div>
          </div>
        </div>
      )}
    </>
  );
};

export default CreateRoom;