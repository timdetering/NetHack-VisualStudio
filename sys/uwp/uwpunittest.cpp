#include "uwp.h"
#include "..\..\win\uwp\winuwp.h"

using namespace Nethack;

static void unit_test_yn_function()
{
    int result;

    g_messageWindow.Init();

    g_testInput.push_back(TestInput('y', "test 1? [yn#aqB] (y) "));
    result = yn_function("test 1?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'y');
    assert(strcmp("test 1? [yn#aqB] (y) y", g_messageWindow.m_toplines.c_str()) == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("test 1? [yn#aqB] (y) y") == 0);

    g_testInput.push_back(TestInput('b', "test 2? [yn#aqB] (y) "));
    result = yn_function("test 2?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'b');
    assert(strcmp("test 2? [yn#aqB] (y) b", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('B', "test 3? [yn#aqB] (y) "));
    result = yn_function("test 3?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'B');
    assert(strcmp("test 3? [yn#aqB] (y) B", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('n', "test 4? [yn#aqB] (y) "));
    result = yn_function("test 4?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');
    assert(strcmp("test 4? [yn#aqB] (y) n", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('1', "test 5? [yn#aqB] (y) ", "test 5? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput('0', "test 5? [yn#aqB] (y) #1", "test 5? [yn#aqB] (y) #1"));
    g_testInput.push_back(TestInput('5', "test 5? [yn#aqB] (y) #10", "test 5? [yn#aqB] (y) #10"));
    g_testInput.push_back(TestInput('\b', "test 5? [yn#aqB] (y) #105", "test 5? [yn#aqB] (y) #105"));
    g_testInput.push_back(TestInput('\n', "test 5? [yn#aqB] (y) #10", "test 5? [yn#aqB] (y) #10"));
    result = yn_function("test 5?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 10);
    assert(strcmp("test 5? [yn#aqB] (y) #10", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('#', "test 6? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput('1', "test 6? [yn#aqB] (y) #"));
    g_testInput.push_back(TestInput('\b', "test 6? [yn#aqB] (y) #1"));
    g_testInput.push_back(TestInput('\n', "test 6? [yn#aqB] (y) #"));
    result = yn_function("test 6?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');
    assert(strcmp("test 6? [yn#aqB] (y) #n", g_messageWindow.m_toplines.c_str()) == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) ", "test 6? [yn#aqB] (y) #n"));
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) ", "test 5? [yn#aqB] (y) #10"));
    g_testInput.push_back(TestInput(kEscape, "test 7? [yn#aqB] (y) ", "test 4? [yn#aqB] (y) n"));
    g_testInput.push_back(TestInput(kEscape, "test 7? [yn#aqB] (y) ", "test 7? [yn#aqB] (y) "));
    result = yn_function("test 7?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(strcmp("test 7? [yn#aqB] (y) q", g_messageWindow.m_toplines.c_str()) == 0);

    iflags.prevmsg_window = 'f';
    g_testInput.push_back(TestInput(kControlP, "test 8? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kSpace, "test 8? [yn#aqB] (y) ", NULL, []() {
        auto line = g_textGrid.ReadScreen(40, 0);
        assert(line.compare(" Message History") == 0);
        line = g_textGrid.ReadScreen(40, 9);
        assert(line.compare(" test 8? [yn#aqB] (y) ") == 0);
        line = g_textGrid.ReadScreen(40, 10);
        assert(line.compare(" --More--") == 0);
    }));
    g_testInput.push_back(TestInput(kEscape, "test 8? [yn#aqB] (y) ", "test 8? [yn#aqB] (y) "));
    result = yn_function("test 8?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(g_messageWindow.m_toplines.compare("test 8? [yn#aqB] (y) q") == 0);
    assert(g_textGrid.IsCornerClear(40, 10));

    g_testInput.push_back(TestInput(kControlP, "test 9? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kEscape, "test 9? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kEscape, "test 9? [yn#aqB] (y) "));
    result = yn_function("test 9?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(g_messageWindow.m_toplines.compare("test 9? [yn#aqB] (y) q") == 0);
    assert(g_textGrid.IsCornerClear(40, 10));

    g_testInput.push_back(TestInput('x', "test 10? "));
    result = yn_function("test 10?", NULL, 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'x');
    assert(g_messageWindow.m_toplines.compare("test 10? x") == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("test 10? x") == 0);

    g_testInput.push_back(TestInput('Y', "test 11? [yn] (y) "));
    result = yn_function("test 11?", "yn", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'y');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 12?", "yn", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 13?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 14?", "abq", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'q');

    g_testInput.push_back(TestInput(kNewline));
    result = yn_function("test 15?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kCarriageReturn));
    result = yn_function("test 16?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 17?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput('5'));
    g_testInput.push_back(TestInput(kEscape));
    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 18?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput('5'));
    g_testInput.push_back(TestInput(kNewline));
    result = yn_function("test 19?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 5);

    g_testInput.push_back(TestInput('c'));
    g_testInput.push_back(TestInput('b'));
    result = yn_function("test 20?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'b');

    g_testInput.push_back(TestInput('5', "test 19? [#] (a) ", "test 19? [#] (a) "));
    g_testInput.push_back(TestInput('9', "test 19? [#] (a) #5", "test 19? [#] (a) #5"));
    g_testInput.push_back(TestInput('8', "test 19? [#] (a) #59", "test 19? [#] (a) #59"));
    g_testInput.push_back(TestInput('7', "test 19? [#] (a) #598", "test 19? [#] (a) #598"));
    g_testInput.push_back(TestInput('6', "test 19? [#] (a) #5987", "test 19? [#] (a) #5987"));
    g_testInput.push_back(TestInput('5', "test 19? [#] (a) #59876", "test 19? [#] (a) #59876"));
    g_testInput.push_back(TestInput('4', "test 19? [#] (a) #598765", "test 19? [#] (a) #598765"));
    g_testInput.push_back(TestInput('3', "test 19? [#] (a) #5987654", "test 19? [#] (a) #5987654"));
    g_testInput.push_back(TestInput('2', "test 19? [#] (a) #59876543", "test 19? [#] (a) #59876543"));
    g_testInput.push_back(TestInput('1', "test 19? [#] (a) #598765432", "test 19? [#] (a) #598765432"));
    g_testInput.push_back(TestInput('4', "test 19? [#] (a) ", "test 19? [#] (a) "));
    g_testInput.push_back(TestInput('2', "test 19? [#] (a) #4", "test 19? [#] (a) #4"));
    g_testInput.push_back(TestInput(kNewline, "test 19? [#] (a) #42", "test 19? [#] (a) #42"));
    result = yn_function("test 19?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 42);
}

static void unit_test_display_message_window()
{
    /* notes: the blocking flag to display_nhwindow() is ignored. */

    pline("Test 1");
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);  assert(line.compare("Test 1--More--") == 0);
    }));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("") == 0);

    pline("Test 2");
    pline("Test 2");
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("Test 2  Test 2--More--") == 0);
    }));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("") == 0);

    pline("Test 3");
    g_testInput.push_back(TestInput(kEscape));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);

}

static void unit_test_putstr()
{
    std::string long_msg("012345678 012345678 012345678 012345678 012345678 01234567.");
    std::string two_line_msg("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678 01234567.");
    std::string three_line_msg(
        "012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678"
        " 012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678"
        " 012345678.");
    std::string long_word_msg("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 test.");

    g_messageWindow.Init();

    /* test wrap */
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 01234567.--More--") == 0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("01234567.--More--") == 0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 2);
        assert(line.compare("012345678.--More--") == 0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("01234567890123456789012345678901234567890123456789012345678901234567890123456789") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("0123456789 test.--More--") == 0);
    }));
    g_messageWindow.Putstr(0, long_msg.c_str());
    g_messageWindow.Putstr(0, two_line_msg.c_str());
    g_messageWindow.Putstr(0, three_line_msg.c_str());
    g_messageWindow.Putstr(0, long_word_msg.c_str());
    auto line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);

    g_testInput.push_back(TestInput(kEscape));
    g_messageWindow.Putstr(0, two_line_msg.c_str());
    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 1);
    assert(line.compare("01234567."));

    /* test accumulation */
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, "test");
    g_messageWindow.Putstr(0, "test");
    g_testInput.push_back(TestInput(kEscape));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);

    g_messageWindow.Putstr(0, "test");
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, "test");
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("test") == 0);
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, long_msg.c_str());
    g_testInput.push_back(TestInput(kEscape));
    g_messageWindow.Putstr(0, "012345678901234567.");
    line = g_textGrid.ReadScreen(0, 0);
    /* last message should not have been written to screen */
    assert(line.compare("") == 0);
    /* but m_toplines should still be accumulating the message */
    assert(g_messageWindow.m_toplines.compare("012345678901234567.") == 0);
    g_messageWindow.Putstr(0, "test.");

    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);
    assert(g_messageWindow.m_toplines.compare("012345678901234567.  test.") == 0);

    /* long message does not fit, should be on its own line */
    g_messageWindow.Putstr(0, long_msg.c_str());
    assert(g_messageWindow.m_toplines.compare(long_msg) == 0);

    /* test clear */
    g_messageWindow.PrepareForInput();
    g_messageWindow.Clear();
    g_messageWindow.Putstr(0, "test.");
    g_messageWindow.PrepareForInput();
    g_messageWindow.Clear();
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);
    g_messageWindow.Putstr(0, "test.");
    g_messageWindow.Putstr(0, "test.");
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("test.  test.") == 0);
    assert(g_testInput.size() == 0);

    //    g_textGrid.Flush();
    //    Sleep(5000);


}

static void unit_test_get_ext_cmd()
{
    g_messageWindow.Init();

    g_testInput.push_back(TestInput('l', "# ", "# ", NULL));
    g_testInput.push_back(TestInput('o', "# loot", "# loot", NULL));
    g_testInput.push_back(TestInput(kNewline, "# loot", "# loot", NULL));
    int cmd = get_ext_cmd();
    assert(strcmp("loot", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);

    iflags.prevmsg_window = 'f';
    g_testInput.push_back(TestInput('j', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# jump", "# jump", NULL));
    g_testInput.push_back(TestInput(kEscape, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(40, 0);
        assert(line.compare(" Message History") == 0);
        line = g_textGrid.ReadScreen(40, 2);
        assert(line.compare(" # loot") == 0);
        line = g_textGrid.ReadScreen(40, 3);
        assert(line.compare(" # jump") == 0);
        line = g_textGrid.ReadScreen(40, 4);
        assert(line.compare(" --More--") == 0);
    }));
    g_testInput.push_back(TestInput(kNewline, "# jump", "# jump", NULL));
    cmd = get_ext_cmd();
    assert(cmd != -1 && strcmp("jump", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput('d', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# dip", NULL));
    cmd = get_ext_cmd();
    assert(cmd != -1 && strcmp("dip", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);
    auto line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput('d', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kEscape, "# dip", "# dip", NULL));
    cmd = get_ext_cmd();
    assert(cmd == -1);
    assert(g_testInput.size() == 0);

}


static void unit_test_message_window()
{
    unit_test_error_output(true);
    unit_test_putstr();
    unit_test_yn_function();
    unit_test_display_message_window();
    unit_test_get_ext_cmd();
}

void unit_test_error_output(bool windowing_initialized)
{
    if (windowing_initialized) {
        assert(iflags.window_inited);

        clear_nhwindow(MESSAGE_WINDOW);

        g_testInput.push_back(TestInput('\n', NULL, NULL, NULL));
        putstr(MESSAGE_WINDOW, 0, "test\n");
        putstr(MESSAGE_WINDOW, 0, "Hit <ENTER> to exit.");
        assert(g_testInput.size() == 0);

        g_testInput.push_back(TestInput('\n', NULL, NULL, NULL));
        uwp_wait_for_return();
        assert(g_testInput.size() == 0);

        clear_nhwindow(MESSAGE_WINDOW);
        g_testInput.push_back(TestInput('\n', NULL, NULL, NULL));
        uwp_wait_for_return();
        assert(g_testInput.size() == 0);

    } else {
        assert(!iflags.window_inited);

        raw_clear_screen();
        raw_printf("test\n");
        raw_printf("Hit <ENTER> to exit.");
        g_testInput.push_back(TestInput('\n', NULL, NULL, NULL));
        uwp_wait_for_return();
        assert(g_testInput.size() == 0);

        raw_clear_screen();
        g_testInput.push_back(TestInput('\n', NULL, NULL, NULL));
        uwp_wait_for_return();
        assert(g_testInput.size() == 0);

    }

}

void unit_tests()
{
    /* we will using windowing so setup windowing system */
    init_nhwindows(NULL, NULL);

    display_gamewindows();

    unit_test_message_window();

    exit_nhwindows((char *)0);
}
