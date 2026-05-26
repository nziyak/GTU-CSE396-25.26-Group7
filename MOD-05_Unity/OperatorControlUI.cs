/// <summary>
/// File:    OperatorControlUI.cs
/// Brief:   Binds MOD-05 operator command buttons and Push-to-Talk UI to RobotManager.
/// </summary>

using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class OperatorControlUI : MonoBehaviour
{
    private const string ForwardCommand = "FORWARD";
    private const string BackwardCommand = "BACKWARD";
    private const string LeftCommand = "LEFT";
    private const string RightCommand = "RIGHT";
    private const string StopCommand = "STOP";

    [Header("Robot")]
    [SerializeField] private RobotManager robotManager;

    [Header("Command Buttons")]
    [SerializeField] private Button forwardButton;
    [SerializeField] private Button backwardButton;
    [SerializeField] private Button leftButton;
    [SerializeField] private Button rightButton;
    [SerializeField] private Button stopButton;

    [Header("Push-to-Talk")]
    [SerializeField] private Button pushToTalkButton;

    private bool pushToTalkTriggersBound;

    private void Awake()
    {
        if (robotManager == null)
        {
            robotManager = FindObjectOfType<RobotManager>();
        }

        AutoBindMissingButtons();
    }

    private void OnEnable()
    {
        BindButtons();
    }

    private void OnDisable()
    {
        UnbindButtons();
    }

    public void SendForward()
    {
        SendCommand(ForwardCommand);
    }

    public void SendBackward()
    {
        SendCommand(BackwardCommand);
    }

    public void SendLeft()
    {
        SendCommand(LeftCommand);
    }

    public void SendRight()
    {
        SendCommand(RightCommand);
    }

    public void SendStop()
    {
        SendCommand(StopCommand);
    }

    public void OnPushToTalkPressed()
    {
        if (robotManager == null)
        {
            Debug.LogWarning("OperatorControlUI: RobotManager is not assigned.");
            return;
        }

        robotManager.StartPushToTalk();
    }

    public void OnPushToTalkReleased()
    {
        if (robotManager == null)
        {
            Debug.LogWarning("OperatorControlUI: RobotManager is not assigned.");
            return;
        }

        robotManager.StopPushToTalk();
    }

    private void BindButtons()
    {
        if (forwardButton != null)
        {
            forwardButton.onClick.AddListener(SendForward);
        }

        if (backwardButton != null)
        {
            backwardButton.onClick.AddListener(SendBackward);
        }

        if (leftButton != null)
        {
            leftButton.onClick.AddListener(SendLeft);
        }

        if (rightButton != null)
        {
            rightButton.onClick.AddListener(SendRight);
        }

        if (stopButton != null)
        {
            stopButton.onClick.AddListener(SendStop);
        }

        if (pushToTalkButton != null)
        {
            BindPushToTalkTriggers();
        }
    }

    private void UnbindButtons()
    {
        if (forwardButton != null)
        {
            forwardButton.onClick.RemoveListener(SendForward);
        }

        if (backwardButton != null)
        {
            backwardButton.onClick.RemoveListener(SendBackward);
        }

        if (leftButton != null)
        {
            leftButton.onClick.RemoveListener(SendLeft);
        }

        if (rightButton != null)
        {
            rightButton.onClick.RemoveListener(SendRight);
        }

        if (stopButton != null)
        {
            stopButton.onClick.RemoveListener(SendStop);
        }
    }

    private void SendCommand(string command)
    {
        if (robotManager == null)
        {
            Debug.LogWarning($"OperatorControlUI: Cannot send '{command}' because RobotManager is not assigned.");
            return;
        }

        robotManager.SendOperatorCommand(command);
    }

    private void BindPushToTalkTriggers()
    {
        if (pushToTalkTriggersBound || pushToTalkButton == null)
        {
            return;
        }

        AddPointerTrigger(pushToTalkButton.gameObject, EventTriggerType.PointerDown, OnPushToTalkPressed);
        AddPointerTrigger(pushToTalkButton.gameObject, EventTriggerType.PointerUp, OnPushToTalkReleased);
        AddPointerTrigger(pushToTalkButton.gameObject, EventTriggerType.PointerExit, OnPushToTalkReleased);
        pushToTalkTriggersBound = true;
    }

    private void AddPointerTrigger(GameObject target, EventTriggerType triggerType, UnityEngine.Events.UnityAction callback)
    {
        EventTrigger trigger = target.GetComponent<EventTrigger>();
        if (trigger == null)
        {
            trigger = target.AddComponent<EventTrigger>();
        }

        for (int i = 0; i < trigger.triggers.Count; i++)
        {
            if (trigger.triggers[i].eventID == triggerType)
            {
                trigger.triggers[i].callback.AddListener(_ => callback());
                return;
            }
        }

        EventTrigger.Entry entry = new EventTrigger.Entry
        {
            eventID = triggerType
        };
        entry.callback.AddListener(_ => callback());
        trigger.triggers.Add(entry);
    }

    private void AutoBindMissingButtons()
    {
        if (forwardButton == null)
        {
            forwardButton = FindButtonByName("ForwardButton", "ButtonForward", "Forward");
        }

        if (backwardButton == null)
        {
            backwardButton = FindButtonByName("BackwardButton", "ButtonBackward", "Backward");
        }

        if (leftButton == null)
        {
            leftButton = FindButtonByName("LeftButton", "ButtonLeft", "Left");
        }

        if (rightButton == null)
        {
            rightButton = FindButtonByName("RightButton", "ButtonRight", "Right");
        }

        if (stopButton == null)
        {
            stopButton = FindButtonByName("StopButton", "ButtonStop", "Stop");
        }

        if (pushToTalkButton == null)
        {
            pushToTalkButton = FindButtonByName("PushToTalkButton", "PTTButton", "PushToTalk", "PTT");
        }
    }

    private Button FindButtonByName(params string[] names)
    {
        Button[] buttons = FindObjectsOfType<Button>(true);
        for (int i = 0; i < buttons.Length; i++)
        {
            for (int j = 0; j < names.Length; j++)
            {
                if (buttons[i].name == names[j])
                {
                    return buttons[i];
                }
            }
        }

        return null;
    }
}
