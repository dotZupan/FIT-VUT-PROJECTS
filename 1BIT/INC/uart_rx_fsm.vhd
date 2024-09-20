-- uart_rx_fsm.vhd: UART controller - finite state machine controlling RX side
-- Author(s): Matus Fignar (xfignam00)

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;



entity UART_RX_FSM is
    port(
       CLK : in std_logic;
       RST : in std_logic;
       DIN : in std_logic;
       CNT16  : in std_logic_vector(4 downto 0);
       CNT8   : in std_logic_vector(3 downto 0);
       RCV_EN : out std_logic;
       CNT_EN : out std_logic;
       VLD_OT : out std_logic
    );
end entity;



architecture behavioral of UART_RX_FSM is
    type t_state is (IDLE, START_BIT, DATA, STOP_BIT, VALID);
    signal state : t_state := IDLE;
    
begin
    --output logic for automat
    CNT_EN <= '1' when state = START_BIT or state = DATA else '0';
    RCV_EN <= '1' when state = DATA else  '0';
    VLD_OT <= '1' when state = VALID else '0';

    --State transitions based on inputs and current states
    process (CLK) begin
        if rising_edge(CLK) then
            case state is
                when IDLE => 
                    if DIN = '0' then
                        state <= START_BIT;
                    end if;
                when START_BIT =>
                    if CNT16 = "01111" then
                        state <= DATA;
                    end if ;
                when DATA =>
                    if CNT8= "1000" then
                        state <= STOP_BIT;
                    end if ;
                when STOP_BIT =>
                    if DIN = '1' then
                        state <= VALID;
                    end if;
                when VALID => 
                        state <= IDLE;
                when others => state <= state;
            end case;
        end if;

    end process;
    

end architecture;
