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
import { Input } from "../components/ui/input"
import { useState } from "react";
import toast from "react-hot-toast";
import { cn } from "@/lib/utils";
import axios from "axios";

const BASE_URL = import.meta.env.VITE_SERVER_URL

type Props = {
  isOpen: boolean;
  setIsOpen: React.Dispatch<React.SetStateAction<boolean>>;
  setOtherOpen: React.Dispatch<React.SetStateAction<boolean>>;
};

const Register: React.FC<Props> = ({
  isOpen,
  setIsOpen,
  setOtherOpen,
}) => {
  const [loading, setLoading] = useState(false);

  const username = localStorage.getItem("username")

  const formSchema = z.object({
    username: z
      .string()
      .min(1, {
        message: "Username is required.",
      }),
    password: z.string().min(1, {
      message: "Password is required.",
    }),
  });

  const form = useForm<z.infer<typeof formSchema>>({
    resolver: zodResolver(formSchema),
    defaultValues: {
      username: "",
      password: "",
    },
  });

  const onSubmit = async (values: z.infer<typeof formSchema>) => {
    if (loading) return;

    if (username){
      toast.error("Already logged in")
      return;
    }

    console.log(values);
    setLoading(true);
    try {
      const res = await axios.post(`${BASE_URL}/auth/register`,values, {
        headers: {
          'Content-Type': 'application/json',
        }
      });
      console.log(res.data);
      if (res.data.message){
        toast.success(res.data.message);
      }
      
      setIsOpen(false);
      form.reset();
    } catch (err: any) {
      console.log(err)
      if (err.response.data.message){
        toast.error(err.response.data.message);
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
            className="relative p-4 w-full max-w-lg max-h-full"
          >
            {/* Modal content */}
            <div className="relative rounded-lg shadow bg-gray-800/90 border-[1px] border-white">
              {/* Modal header */}
              <div className="flex items-center justify-between p-4 md:p-5 border-b rounded-t border-gray-600">
                <h3 className="text-2xl font-semibold text-white">
                  Register to our game
                </h3>
                <button
                  onClick={() => setIsOpen(false)}
                  type="button"
                  className="end-2.5 text-gray-400 bg-transparent rounded-lg text-sm w-8 h-8 ms-auto inline-flex justify-center items-center hover:bg-gray-600 hover:text-white"
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
              <div className="p-4 md:px-6 md:py-4">
                <Form {...form}>
                  <form
                    className="space-y-6"
                    onSubmit={form.handleSubmit(onSubmit)}
                  >
                    <FormField
                      control={form.control}
                      name="username"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel className="block mb-2 text-sm font-medium text-white text-start">
                            Username
                          </FormLabel>
                          <FormControl>
                            <Input
                              {...field}
                              className="text-sm rounded-lg block w-full p-2.5 bg-gray-600 border-gray-500 focus:border-gray-600 placeholder-gray-400 text-white ring-offset-gray-400"
                              placeholder="your_username"
                            />
                          </FormControl>
                          <FormMessage />
                        </FormItem>
                      )}
                    />
                    <FormField
                      control={form.control}
                      name="password"
                      render={({ field }) => (
                        <FormItem>
                          <FormLabel className="block mb-2 text-sm font-medium text-white text-start">
                            Password
                          </FormLabel>
                          <FormControl>
                            <Input
                              {...field}
                              type="password"
                              placeholder="••••••••"
                              className="text-sm rounded-lg block w-full p-2.5 bg-gray-600 border-gray-500 focus:border-gray-600 placeholder-gray-400 text-white ring-offset-gray-400"
                            />
                          </FormControl>
                          <FormMessage />
                        </FormItem>
                      )}
                    />

                    <button
                      type="submit"
                      className={cn(
                        "w-full text-white focus:ring-4 focus:outline-none focus:ring-blue-300 font-medium rounded-lg text-sm px-5 py-2.5 text-center bg-sky-500 hover:bg-sky-600",
                        {
                          "bg-gray-600 hover:bg-gray-600 select-none cursor-progress":
                            loading,
                        }
                      )}
                      disabled={loading}
                    >
                      Create your new account
                    </button>
                    <div className="text-sm font-medium text-gray-300">
                      Already have an account?{" "}
                      <a
                        href="#"
                        onClick={() => {
                          setIsOpen(false);
                          setOtherOpen(true);
                        }}
                        className="text-sky-300 hover:underline"
                      >
                        Login
                      </a>
                    </div>
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

export default Register;
