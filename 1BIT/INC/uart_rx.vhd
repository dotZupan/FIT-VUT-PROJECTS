-- uart_rx.vhd: UART controller - receiving (RX) side
-- Author(s): Matus Fignar (xfignam00)

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;



-- Entity declaration (DO NOT ALTER THIS PART!)
entity UART_RX is
    port(
        CLK      : in std_logic;
        RST      : in std_logic;
        DIN      : in std_logic;
        DOUT     : out std_logic_vector(7 downto 0);
        DOUT_VLD : out std_logic
    );
end entity;


-- Architecture implementation (INSERT YOUR IMPLEMENTATION HERE)
architecture behavioral of UART_RX is
    signal cnt_en : std_logic := '0';
    signal rcv_en : std_logic := '0';
    signal cnt_16 : std_logic_vector(4 downto 0) := "01000";
    signal cnt_8  : std_logic_vector(3 downto 0) := "0000";
    signal vld    : std_logic := '0';

begin 
    -- Instance of RX FSM
    fsm: entity work.UART_RX_FSM
    port map (
        CLK => CLK,
        RST => RST,
        DIN => DIN,
        CNT16 => cnt_16,
        CNT8 => cnt_8,
        RCV_EN => rcv_en,
        CNT_EN => cnt_en,
        VLD_OT => vld
    );
    DOUT_VLD <= vld;
    

     p_cnt_16 : process(CLK, RST, cnt_en)
     begin
          if RST = '1' or cnt_en = '0' then
               cnt_16 <= "01000";
          elsif rising_edge(CLK) then
               if cnt_en = '1' then
                    if cnt_16 = "01111" then 
                         cnt_16 <= "00000";
                    else
                         cnt_16 <= cnt_16 +1;
                    end if;
               end if;
          end if;
     end process p_cnt_16;


     p_cnt_8 : process(CLK, RST, rcv_en, cnt_16)
     begin
          if (RST = '1' or rcv_en = '0') then
               cnt_8 <= "0000";
          elsif rising_edge(CLK) then
               if (cnt_16 = "01111" and rcv_en = '1') then
                    cnt_8 <= cnt_8 + 1;
               end if;
          end if;
     end process p_cnt_8;


     p_d_reg : process(CLK, RST, DIN, rcv_en, cnt_16)
     begin
          if (RST = '1') then
               DOUT <= (others => '0');
          elsif rising_edge(CLK) then
               if (rcv_en = '1' and cnt_16 = "01111") then
                    case cnt_8 is
                         when "0000" => DOUT(0) <= DIN;
                         when "0001" => DOUT(1) <= DIN;
                         when "0010" => DOUT(2) <= DIN;
                         when "0011" => DOUT(3) <= DIN;
                         when "0100" => DOUT(4) <= DIN;
                         when "0101" => DOUT(5) <= DIN;
                         when "0110" => DOUT(6) <= DIN;
                         when "0111" => DOUT(7) <= DIN;
                         when others => null;
                    end case;
               end if;
          end if;
     end process p_d_reg;
end architecture;
