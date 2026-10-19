/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "../deskflow/MockEventQueue.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "client/Client.h"
#include "client/ServerProxy1_9.h"
#include "common/Settings.h"
#include "deskflow/AppUtil.h"
#include "deskflow/BidirectionalInput.h"
#include "deskflow/Computer.h"
#include "deskflow/PlatformComputer.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "io/IStream.h"
#include "net/ISocketFactory.h"
#include "server/ClientProxy1_9.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"
#include <QTemporaryDir>
#include <QTest>
#include <cstring>

namespace {
using deskflow::SharedInput;
using Kind = SharedInput::Kind;
class TestAppUtil : public AppUtil
{
public:
  int run() override
  {
    return 0;
  }
  std::vector<std::string> getKeyboardLayoutList() override
  {
    return {"en"};
  }
  std::string getCurrentLanguageCode() override
  {
    return "en";
  }
};
class TestQueue : public MockEventQueue
{
public:
  void addHandler(EventTypes type, void *target, const EventHandler &fn) override
  {
    handlers[{type, target}] = fn;
  }
  void removeHandler(EventTypes type, void *target) override
  {
    handlers.erase({type, target});
  }
  bool dispatchEvent(const Event &e) override
  {
    auto it = handlers.find({e.getType(), e.getTarget()});
    if (it == handlers.end())
      return false;
    auto fn = it->second;
    fn(e);
    return true;
  }
  void addEvent(Event &&e) override
  {
    if (e.getFlags() & Event::EventFlags::DeliverImmediately)
      dispatchEvent(e);
    Event::deleteData(e);
  }
  std::map<std::pair<EventTypes, void *>, EventHandler> handlers;
};
class TestPlatform : public PlatformComputer
{
public:
  explicit TestPlatform(IEventQueue *events) : PlatformComputer(events)
  {
  }
  void *getEventTarget() const override
  {
    return const_cast<TestPlatform *>(this);
  }
  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }
  void getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const override
  {
    x = y = 0;
    w = 1920;
    h = 1080;
  }
  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = cursorX;
    y = cursorY;
  }
  void reconfigure(uint32_t sides) override
  {
    m_sides = sides;
  }
  uint32_t activeSides() override
  {
    return m_sides;
  }
  void warpCursor(int32_t x, int32_t y) override
  {
    cursorX = x;
    cursorY = y;
    calls.push_back("move");
  }
  uint32_t registerHotKey(KeyID, KeyModifierMask) override
  {
    return ++hotkeyId;
  }
  void unregisterHotKey(uint32_t) override
  {
  }
  void fakeInputBegin() override
  {
    calls.push_back("begin");
  }
  void fakeInputEnd() override
  {
    calls.push_back("end");
  }
  int32_t getJumpZoneSize() const override
  {
    return 1;
  }
  bool isAnyMouseButtonDown(uint32_t &) const override
  {
    return false;
  }
  void getCursorCenter(int32_t &x, int32_t &y) const override
  {
    x = 960;
    y = 540;
  }
  void fakeMouseButton(ButtonID, bool press) override
  {
    calls.push_back(press ? "button-down" : "button-up");
    buttonPositions.emplace_back(cursorX, cursorY);
  }
  void fakeMouseMove(int32_t x, int32_t y) override
  {
    warpCursor(x, y);
  }
  void fakeMouseRelativeMove(int32_t, int32_t) const override
  {
  }
  void fakeMouseWheel(ScrollDelta) const override
  {
    calls.push_back("wheel");
  }
  void updateKeyMap() override
  {
  }
  void updateKeyState() override
  {
  }
  void setHalfDuplexMask(KeyModifierMask) override
  {
  }
  void fakeKeyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
    calls.push_back("key-down");
  }
  bool fakeKeyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &) override
  {
    calls.push_back("repeat");
    return true;
  }
  bool fakeKeyUp(KeyButton) override
  {
    calls.push_back("key-up");
    return true;
  }
  void fakeAllKeysUp() override
  {
    calls.push_back("release-keys");
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  bool isKeyDown(KeyButton) const override
  {
    return false;
  }
  KeyModifierMask getActiveModifiers() const override
  {
    return 0;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  int32_t pollActiveGroup() const override
  {
    return 0;
  }
  void pollPressedKeys(KeyButtonSet &) const override
  {
  }
  void clearStaleModifiers() override
  {
  }
  void enable() override
  {
  }
  void disable() override
  {
  }
  void enter() override
  {
  }
  bool canLeave() override
  {
    return true;
  }
  void leave() override
  {
  }
  bool setClipboard(ClipboardID, const IClipboard *) override
  {
    return false;
  }
  void checkClipboards() override
  {
  }
  void openScreensaver(bool) override
  {
  }
  void closeScreensaver() override
  {
  }
  void screensaver(bool) override
  {
  }
  void resetOptions() override
  {
  }
  void setOptions(const OptionsList &) override
  {
  }
  void setSequenceNumber(uint32_t) override
  {
  }
  bool isPrimary() const override
  {
    return true;
  }
  std::string getSecureInputApp() const override
  {
    return {};
  }
  int32_t cursorX = 960, cursorY = 540;
  mutable QStringList calls;
  std::vector<std::pair<int32_t, int32_t>> buttonPositions;

protected:
  void updateButtons() override
  {
  }
  IKeyState *getKeyState() const override
  {
    return nullptr;
  }
  void handleSystemEvent(const Event &) override
  {
  }

private:
  uint32_t m_sides = 0, hotkeyId = 0;
};
class TestStream : public deskflow::IStream
{
public:
  void close() override
  {
  }
  uint32_t read(void *buffer, uint32_t size) override
  {
    auto n = std::min(size, static_cast<uint32_t>(input.size()));
    std::memcpy(buffer, input.constData(), n);
    input.remove(0, n);
    return n;
  }
  void write(const void *buffer, uint32_t size) override
  {
    output.append(static_cast<const char *>(buffer), size);
  }
  void flush() override
  {
  }
  void shutdownInput() override
  {
  }
  void shutdownOutput() override
  {
  }
  void *getEventTarget() const override
  {
    return const_cast<TestStream *>(this);
  }
  bool isReady() const override
  {
    return !input.isEmpty();
  }
  uint32_t getSize() const override
  {
    return static_cast<uint32_t>(input.size());
  }
  QByteArray input, output;
};
class TestSocketFactory : public ISocketFactory
{
public:
  IDataSocket *create(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    return nullptr;
  }
  IListenSocket *createListen(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    return nullptr;
  }
};
class MotionServerProxy : public ServerProxy
{
public:
  using ServerProxy::ServerProxy;
protected:
  ConnectionResult parseHandshakeMessage(const uint8_t *code) override
  {
    return parseMessage(code);
  }
};
class TestPeer : public ClientProxy1_9
{
public:
  using ClientProxy1_9::ClientProxy1_9;
  void getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const override
  {
    x = y = 0;
    w = 1920;
    h = 1080;
  }
  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 960;
    y = 540;
  }
  bool parseInput()
  {
    return parseMessage(reinterpret_cast<const uint8_t *>(kMsgDBidirectionalInput));
  }
};
struct Fixture
{
  TestQueue events;
  TestPlatform *platform = new TestPlatform(&events);
  deskflow::Computer computer{platform, &events};
  PrimaryClient primary{"host", &computer};
  deskflow::server::Config config{&events};
  std::unique_ptr<Server> server;
  TestStream *stream = nullptr;
  TestPeer *peer = nullptr;
  Fixture()
  {
    config.addComputer("host");
    config.addComputer("Mac");
    config.connect("host", Direction::Right, 0, 1, "Mac", 0, 1);
    config.connect("Mac", Direction::Left, 0, 1, "host", 0, 1);
    InputFilter::Rule mac(new InputFilter::KeystrokeCondition(&events, kKeyF2, 0));
    mac.adoptAction(new InputFilter::SwitchToComputerAction(&events, "Mac"), true);
    config.getInputFilter()->addFilterRule(mac);
    InputFilter::Rule host(new InputFilter::KeystrokeCondition(&events, kKeyF1, 0));
    host.adoptAction(new InputFilter::SwitchToComputerAction(&events, "host"), true);
    config.getInputFilter()->addFilterRule(host);
    InputFilter::Rule release(new InputFilter::KeystrokeCondition(&events, kKeyF3, 0));
    release.adoptAction(new InputFilter::SwitchToComputerAction(&events, "host"), false);
    config.getInputFilter()->addFilterRule(release);
    server = std::make_unique<Server>(config, &primary, &computer, &events);
    stream = new TestStream;
    peer = new TestPeer("Mac", stream, server.get(), &events);
    server->adoptClient(peer);
    platform->calls.clear();
    stream->output.clear();
  }
  void switchTo(const std::string &name)
  {
    Server::SwitchToComputerInfo info(name);
    events.dispatchEvent(
        Event(EventTypes::ServerSwitchToComputer, config.getInputFilter(), &info, Event::EventFlags::DontFreeData)
    );
  }
};
SharedInput key(Kind kind, uint32_t epoch = 0)
{
  SharedInput i;
  i.kind = kind;
  i.key = 'a';
  i.button = 30;
  i.epoch = epoch;
  return i;
}
} // namespace

class BidirectionalInputTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase()
  {
    m_arch = std::make_unique<Arch>();
#ifdef Q_OS_WIN
    m_arch->init();
#endif
    Settings::setSettingsFile(m_directory.filePath("Deskflow.conf"));
    Settings::setStateFile(m_directory.filePath("Deskflow.state"));
    Settings::setValue(Settings::Core::BidirectionalInput, true);
  }
  void ownershipHandoffAndDrag()
  {
    deskflow::InputOwnership owner;
    QVERIFY(owner.accept("Mac", key(Kind::KeyDown)));
    QVERIFY(!owner.accept("Windows", key(Kind::KeyDown)));
    QVERIFY(!owner.accept("Windows", key(Kind::KeyUp)));
    QVERIFY(owner.accept("Mac", key(Kind::KeyRepeat)));
    QVERIFY(owner.accept("Mac", key(Kind::KeyUp)));
    QVERIFY(owner.accept("Windows", key(Kind::KeyDown)));
    owner.clear();
    QVERIFY(!owner.accept("Mac", key(Kind::KeyUp)));
    SharedInput drag;
    drag.kind = Kind::ButtonDown;
    drag.button = 1;
    QVERIFY(owner.accept("Mac", drag));
    SharedInput motion;
    motion.kind = Kind::RelativeMotion;
    QVERIFY(!owner.accept("Windows", motion));
    drag.kind = Kind::ButtonUp;
    QVERIFY(owner.accept("Mac", drag));
    QVERIFY(owner.accept("Windows", motion));
  }
  void remoteInputInjectsOnServerAndReleases()
  {
    Fixture f;
    f.server->receiveInput(f.peer, key(Kind::KeyDown));
    f.server->receiveInput(f.peer, key(Kind::KeyRepeat));
    f.server->receiveInput(f.peer, key(Kind::KeyUp));
    QCOMPARE(
        f.platform->calls, QStringList({"begin", "key-down", "end", "begin", "repeat", "end", "begin", "key-up", "end"})
    );
  }
  void focusChangeReleasesAndRejectsStaleInput()
  {
    Fixture f;
    f.server->receiveInput(f.peer, key(Kind::KeyDown));
    f.platform->calls.clear();
    f.switchTo("Mac");
    QVERIFY(f.platform->calls.contains("key-up"));
    f.stream->output.clear();
    f.server->receiveInput(f.peer, key(Kind::KeyDown, 0)); // obsolete focus
    QVERIFY(f.stream->output.isEmpty());
    f.server->receiveInput(f.peer, key(Kind::KeyDown, 1)); // locally delivered, never echo
    QVERIFY(f.stream->output.isEmpty());
    f.server->receiveInput(f.peer, key(Kind::KeyUp, 1));
    f.switchTo("host");
    f.platform->calls.clear();
    f.server->receiveInput(f.peer, key(Kind::KeyDown, 2));
    QVERIFY(f.platform->calls.contains("key-down"));
  }
  void reverseMouseMovesAcrossEdges()
  {
    Fixture f;
    SharedInput input;
    input.kind = Kind::RelativeMotion;
    input.x = 12;
    input.y = -7;
    f.server->receiveInput(f.peer, input);
    QCOMPARE(f.platform->cursorX, 972);
    QCOMPARE(f.platform->cursorY, 533);
    f.switchTo("Mac");
    f.stream->output.clear();
    input.kind = Kind::AbsoluteMotion;
    input.epoch = 1;
    input.x = 0;
    input.y = 540;
    f.server->receiveInput(f.peer, input);
    QVERIFY(f.stream->output.contains("DBFC"));
    QVERIFY(f.stream->output.contains(QByteArray("DBFC") + QByteArray::fromHex("00000002 00")));
  }
  void disconnectReleasesKeysHeldOnServer()
  {
    Fixture f;
    f.server->receiveInput(f.peer, key(Kind::KeyDown));
    f.platform->calls.clear();
    f.events.dispatchEvent(Event(EventTypes::ClientProxyDisconnected, f.peer));
    QVERIFY(f.platform->calls.contains("key-up"));
  }
  void disabledModeIgnoresReverseInput()
  {
    Settings::setValue(Settings::Core::BidirectionalInput, false);
    {
      Fixture f;
      f.server->receiveInput(f.peer, key(Kind::KeyDown));
      QVERIFY(f.platform->calls.isEmpty());
    }
    Settings::setValue(Settings::Core::BidirectionalInput, true);
  }
  void protocolRoundTripAndMalformedInput()
  {
    Fixture f;
    auto input = key(Kind::KeyDown);
    input.language = "en";
    TestStream wire;
    // Exercise the real client encoder and server decoder together.
    ServerProxy1_9 encoder(reinterpret_cast<Client *>(0x1), &wire, &f.events);
    encoder.sendInput(input);
    f.stream->input = wire.output.mid(4);
    QVERIFY(f.peer->parseInput());
    QVERIFY(f.platform->calls.contains("key-down"));
    wire.output.clear();
    ProtocolUtil::writef(&wire, kMsgDBidirectionalInput, 0, 255, 0, 0, 0, 1, 0, 0, &input.language);
    f.stream->input = wire.output.mid(4);
    QVERIFY_EXCEPTION_THROWN(f.peer->parseInput(), BadClientException);
  }
  void remoteHotkeysChangeFocusAndRejectUnpairedRelease()
  {
    Fixture f;
    SharedInput input;
    input.kind = Kind::HotkeyDown;
    input.key = kKeyF2;
    f.server->receiveInput(f.peer, input);
    QVERIFY(f.stream->output.contains("CINN"));
    f.stream->output.clear();
    f.server->receiveInput(f.peer, input); // duplicate press, and obsolete epoch
    QVERIFY(f.stream->output.isEmpty());
    input.kind = Kind::HotkeyUp;
    f.server->receiveInput(f.peer, input); // paired release across focus change
    input.kind = Kind::HotkeyDown;
    input.key = kKeyF1;
    input.epoch = 1;
    f.server->receiveInput(f.peer, input);
    QVERIFY(f.stream->output.contains("COUT"));
    QVERIFY(f.stream->output.contains(QByteArray("DBFC") + QByteArray::fromHex("00000002 00")));
    QVERIFY(!f.peer->trackHotkey(kKeyF2, 0, false)); // previous release was tracked
    QVERIFY(!f.peer->trackHotkey(kKeyF3, 0, false)); // never pressed
  }
  void clientCaptureFollowsFocusAndRestoresOnDisconnect()
  {
    TestQueue events;
    auto *platform = new TestPlatform(&events);
    deskflow::Computer computer(platform, &events);
    {
      Client client(&events, "Mac", NetworkAddress(), new TestSocketFactory, &computer);
      client.setBidirectionalAllowed(true);
      client.setInputHotkeys({kKeyF1, 0, kKeyF2, 0});
      client.setInputFocus(0, false);
      client.handshakeComplete();
      QVERIFY(!computer.isOnComputer());
      client.setInputFocus(1, true);
      QVERIFY(computer.isOnComputer());
      client.setInputFocus(2, false);
      QVERIFY(!computer.isOnComputer());
      QVERIFY(platform->calls.contains("release-keys"));
      client.disconnect(nullptr);
      QVERIFY(computer.isOnComputer());
    }
    QVERIFY(computer.isOnComputer());
  }
  void receiveBatchFlushesMotionBeforeYieldAndPreservesClickPosition()
  {
    TestQueue events;
    auto *platform = new TestPlatform(&events);
    deskflow::Computer computer(platform, &events);
    Client client(&events, "Mac", NetworkAddress(), new TestSocketFactory, &computer);
    client.setBidirectionalAllowed(true);
    client.handshakeComplete();
    client.enter(0, 0, 1, 0, false);
    platform->calls.clear();
    TestStream wire;
    MotionServerProxy proxy(&client, &wire, &events);
    for (int x = 1; x <= 130; ++x)
      ProtocolUtil::writef(&wire, kMsgDMouseMove, x, 20);
    ProtocolUtil::writef(&wire, kMsgDMouseDown, 1);
    ProtocolUtil::writef(&wire, kMsgDMouseMove, 150, 30);
    ProtocolUtil::writef(&wire, kMsgDMouseUp, 1);
    wire.input = wire.output;
    wire.output.clear();
    QVERIFY(events.dispatchEvent(Event(EventTypes::StreamInputReady, wire.getEventTarget())));
    QVERIFY(wire.isReady());
    QVERIFY(platform->cursorX > 0 && platform->cursorX <= 64);
    // The fake queue discards deferred events; explicitly dispatch each turn.
    for (int i = 0; wire.isReady() && i < 134; ++i)
      QVERIFY(events.dispatchEvent(Event(EventTypes::StreamInputReady, wire.getEventTarget())));
    QVERIFY(!wire.isReady());
    QCOMPARE(platform->cursorX, 150);
    QCOMPARE(platform->cursorY, 30);
    QCOMPARE(platform->buttonPositions.size(), size_t(2));
    QCOMPARE(platform->buttonPositions[0].first, 130);
    QCOMPARE(platform->buttonPositions[0].second, 20);
    QCOMPARE(platform->buttonPositions[1].first, 150);
    QCOMPARE(platform->buttonPositions[1].second, 30);
  }
  void pairedHotkeyReleaseCompletesAfterAnotherSourceTakesControl()
  {
    Fixture f;
    SharedInput input;
    input.kind = Kind::HotkeyDown;
    input.key = kKeyF3;
    f.server->receiveInput(f.peer, input);
    f.switchTo("Mac");
    auto *down = IPlatformComputer::KeyInfo::alloc('a', 0, 30, 1);
    f.events.addEvent(Event(EventTypes::KeyStateKeyDown, f.platform, down, Event::EventFlags::DeliverImmediately));
    f.stream->output.clear();
    input.kind = Kind::HotkeyUp; // belongs to the earlier press, even with obsolete epoch
    f.server->receiveInput(f.peer, input);
    QVERIFY(f.stream->output.contains("COUT"));
    QVERIFY(!f.peer->trackHotkey(kKeyF3, 0, false));
  }

private:
  QTemporaryDir m_directory;
  std::unique_ptr<Arch> m_arch;
  TestAppUtil m_appUtil;
  Log m_log;
};
QTEST_MAIN(BidirectionalInputTests)
#include "BidirectionalInputTests.moc"
