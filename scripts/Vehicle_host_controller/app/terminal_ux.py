class Terminal_ux():
    
    def __init__(self) -> None:
        pass
    
    @staticmethod
    def _print_and_execute_menu(menu_dict: dict) -> bool:
        dict_keys_list = list(menu_dict.keys())

        for ind, desc in enumerate(dict_keys_list):
            print(f"[{ind}] {desc}")
        
        user_input = -1
        while int(user_input) not in range(len(dict_keys_list)):
            user_input = input("Enter the number of wanted option: ")

            if len(user_input) == 0:
                return False

            if int(user_input) not in range(len(dict_keys_list)):
                print("\nInvalid input! Try again...\n")

        action_key = dict_keys_list[int(user_input)]

        print()

        menu_dict[action_key]()

        return True
        
        
